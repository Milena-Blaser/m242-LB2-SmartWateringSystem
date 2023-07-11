package ch.alptbz.mqttgrafanademo;

import com.influxdb.client.InfluxDBClient;
import com.influxdb.client.InfluxDBClientFactory;
import com.influxdb.client.WriteApiBlocking;
import com.influxdb.client.domain.WritePrecision;
import com.influxdb.client.write.Point;
import org.eclipse.paho.client.mqttv3.MqttAsyncClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.io.FileReader;
import java.io.IOException;
import java.time.Instant;
import java.util.Properties;
import java.util.function.BiConsumer;
import java.util.logging.ConsoleHandler;
import java.util.logging.Level;
import java.util.logging.Logger;

public class Main {
    public static double humidity = 0;
    private static Logger logger;
    private static Properties config;
    private static InfluxDBClient client;
    private static String influxdbBucket;
    private static String influxdbOrg;

    private static boolean loadConfig() {
        config = new Properties();
        try {
            config.load(new FileReader("config.properties"));
            return true;
        } catch (IOException e) {
            logger.log(Level.SEVERE, "Error loading config file",e);
        }
        return false;
    }

    public final static void main(String[] args) throws InterruptedException {
        ConsoleHandler ch = new ConsoleHandler();
        ch.setLevel(Level.ALL);
        Logger.getGlobal().addHandler(ch);

        logger = Logger.getLogger("main");

        if(!loadConfig()) return;

        logger.info("Config file loaded");

        client = InfluxDBClientFactory.create(config.getProperty("influxdb-url"),
                config.getProperty("influxdb-token").toCharArray());

        influxdbBucket = config.getProperty("influxdb-bucket");
        influxdbOrg = config.getProperty("influxdb-org");

        Mqtt mqttClient = new Mqtt(config.getProperty("mqtt-url"), MqttAsyncClient.generateClientId());
        try {
            mqttClient.start();
            mqttClient.subscribe("SmartWateringSystem/#");
        } catch (MqttException e) {
            e.printStackTrace();
        }

        mqttClient.addHandler(new BiConsumer<String, MqttMessage>() {
            @Override
            public void accept(String s, MqttMessage mqttMessage) {
                if(s.equals("SmartWateringSystem/Humidity")) {
                    humidity = Double.parseDouble(mqttMessage.toString());
                }
            }
        });

        double lastHumidity = humidity;

        while(true) {
            if(Math.abs(lastHumidity - humidity) >= 1)  {
                System.out.println("Humidity changed. Current: "+humidity);
                Point point = Point
                        .measurement("plant-status")
                        .addTag("host", "host1")
                        .addField("humidity", humidity)
                        .time(Instant.now(), WritePrecision.NS);

                WriteApiBlocking writeApi = client.getWriteApiBlocking();
                writeApi.writePoint(influxdbBucket, influxdbOrg, point);

                lastHumidity = humidity;
            }
        }

    }

}

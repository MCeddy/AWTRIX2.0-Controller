void connectToMqtt()
{
    log("Connecting to MQTT...");

    isMqttConnecting = true;
    mqttClient.connect();
}

void reconnect()
{
    if (isMqttConnecting)
    {
        return;
    }

    log("reconnecting to " + String(mqtt_server) + ":" + String(mqtt_port));

    String clientId = "SmartDisplay-";
    clientId += String(random(0xffff), HEX);

    //hardwareAnimatedSearch(1, 28, 0);

    // connect to MQTT broker
    isMqttConnecting = true;
    mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
    isMqttConnecting = false;

    log("connected to MQTT broker");

    mqttClient.subscribe("smartDisplay/client/in/#", 1);
    mqttClient.publish("smartDisplay/client/out/connected", 1, false, "");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    isMqttConnecting = false;

    log("Disconnected from MQTT.");

    /*if (WiFi.isConnected())
	{
		mqttReconnectTimer.once(2, connectToMqtt);
	}*/
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    if (updating)
    {
        // ignore
        return;
    }

    isMqttConnecting = false;

    String s_payload = String(payload);
    String s_topic = String(topic);
    int last = s_topic.lastIndexOf("/") + 1;
    String channel = s_topic.substring(last);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, s_payload);

    log("MQTT topic: " + s_topic);
    log("MQTT payload: " + s_payload);

    processing(channel, doc);
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
    log("Connected to WiFi.");

    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    log("Disconnected from WiFi.");

    //mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    //wifiReconnectTimer.once(2, connectToWifi);
}
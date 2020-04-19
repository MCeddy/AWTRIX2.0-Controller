void saveConfigCallback()
{
    log("Should save config");

    shouldSaveConfig = true;
}

bool saveConfig()
{
    DynamicJsonDocument doc(1024);
    doc["mqtt_server"] = mqtt_server;
    doc["mqtt_port"] = mqtt_port;
    doc["mqtt_user"] = mqtt_user;
    doc["mqtt_password"] = mqtt_password;
    doc["MatrixType"] = MatrixType2;
    //doc["matrixCorrection"] = matrixTempCorrection;

    File configFile = SPIFFS.open(CONFIG_FILE, "w");
    if (!configFile)
    {
        log("saveConfig: failed to open config file for writing");
        delay(1000);

        return false;
    }

    serializeJson(doc, configFile);

    configFile.close();

    return true;
}

void loadConfig(DynamicJsonDocument doc)
{
    strcpy(mqtt_server, doc["mqtt_server"]);
    mqtt_port = doc["mqtt_port"].as<int>();
    strcpy(mqtt_user, doc["mqtt_user"]);
    strcpy(mqtt_password, doc["mqtt_password"]);
    MatrixType2 = doc["MatrixType"].as<bool>();
    //matrixTempCorrection = doc["matrixCorrection"].as<int>();

    configLoaded = true;
}
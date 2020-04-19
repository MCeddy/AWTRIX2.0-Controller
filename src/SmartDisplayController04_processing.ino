void processing(String type, DynamicJsonDocument doc)
{
    lastMessageFromServer = millis();

    log("MQTT type: " + type);

    String jsonOutput;
    serializeJson(doc, jsonOutput);
    log("MQTT json: " + jsonOutput);

    if (type.equals("show"))
    {
        if (!powerOn)
        {
            return;
        }

        matrix->show();
    }
    else if (type.equals("clear"))
    {
        if (!powerOn)
        {
            return;
        }

        matrix->clear();
    }
    else if (type.equals("drawText"))
    {
        if (!powerOn)
        {
            return;
        }

        if (doc["font"].as<String>().equals("big"))
        {
            matrix->setFont();
            matrix->setCursor(doc["x"].as<int16_t>(), doc["y"].as<int16_t>() - 1);
        }
        else
        {
            matrix->setFont(&TomThumb);
            matrix->setCursor(doc["x"].as<int16_t>(), doc["y"].as<int16_t>() + 5);
        }
        matrix->setTextColor(matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));

        String text = doc["text"];
        matrix->print(utf8ascii(text));
    }
    else if (type.equals("drawBMP"))
    {
        if (!powerOn)
        {
            return;
        }

        int16_t h = doc["height"].as<int16_t>();
        int16_t w = doc["width"].as<int16_t>();
        int16_t x = doc["x"].as<int16_t>();
        int16_t y = doc["y"].as<int16_t>();

        for (int16_t j = 0; j < h; j++, y++)
        {
            for (int16_t i = 0; i < w; i++)
            {
                matrix->drawPixel(x + i, y, doc["bmp"][j * w + i].as<int16_t>());
            }
        }
    }
    else if (type.equals("drawLine"))
    {
        if (!powerOn)
        {
            return;
        }

        matrix->drawLine(doc["x0"].as<int16_t>(), doc["y0"].as<int16_t>(), doc["x1"].as<int16_t>(), doc["y1"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
    }
    else if (type.equals("drawCircle"))
    {
        if (!powerOn)
        {
            return;
        }

        matrix->drawCircle(doc["x0"].as<int16_t>(), doc["y0"].as<int16_t>(), doc["r"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
    }
    else if (type.equals("drawRect"))
    {
        if (!powerOn)
        {
            return;
        }

        matrix->drawRect(doc["x"].as<int16_t>(), doc["y"].as<int16_t>(), doc["w"].as<int16_t>(), doc["h"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
    }
    else if (type.equals("fill"))
    {
        if (!powerOn)
        {
            return;
        }

        matrix->fillScreen(matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
    }
    else if (type.equals("drawPixel"))
    {
        if (!powerOn)
        {
            return;
        }

        matrix->drawPixel(doc["x"].as<int16_t>(), doc["y"].as<int16_t>(), matrix->Color(doc["color"][0].as<int16_t>(), doc["color"][1].as<int16_t>(), doc["color"][2].as<int16_t>()));
    }
    else if (type.equals("reset"))
    {
        ESP.reset();
    }
    else if (type.equals("resetSettings"))
    {
        wifiManager.resetSettings();
        ESP.reset();
    }
    else if (type.equals("changeSettings"))
    {
        if (doc.containsKey("mqtt_server"))
        {
            strcpy(mqtt_server, doc["mqtt_server"]);
        }

        if (doc.containsKey("mqtt_port"))
        {
            mqtt_port = doc["mqtt_port"].as<int>();
        }

        if (doc.containsKey("mqtt_user"))
        {
            strcpy(mqtt_user, doc["mqtt_user"]);
        }

        if (doc.containsKey("mqtt_password"))
        {
            strcpy(mqtt_password, doc["mqtt_password"]);
        }

        matrix->clear();
        matrix->setCursor(6, 6);
        matrix->setTextColor(matrix->Color(0, 255, 50));
        matrix->print("SAVE");
        matrix->show();

        delay(2000);

        if (saveConfig())
        {
            ESP.reset();
        }
    }
    else if (type.equals("power"))
    {
        bool oldValue = powerOn;
        powerOn = doc["on"].as<bool>();

        if (oldValue && !powerOn)
        {
            // switched power off
            poweringOff = true;
        }
    }
    else if (type.equals("ping"))
    {
        if (USBConnection)
        {
            Serial.println("ping");
        }
        else
        {
            mqttClient.publish("smartDisplay/client/out/ping", 0, true, "pong");
        }
    }
}
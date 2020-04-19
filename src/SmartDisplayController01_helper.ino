void log(const String &message)
{
    if (USBConnection)
    {
        return;
    }

    Serial.println(message);
}

int GetRSSIasQuality(int rssi)
{
    int quality = 0;

    if (rssi <= -100)
    {
        quality = 0;
    }
    else if (rssi >= -50)
    {
        quality = 100;
    }
    else
    {
        quality = 2 * (rssi + 100);
    }
    return quality;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
    log("Entered config mode");
    log(myWiFiManager->getConfigPortalSSID());

    matrix->clear();
    matrix->setCursor(3, 6);
    matrix->setTextColor(matrix->Color(0, 255, 50));
    matrix->print("Hotspot");
    matrix->show();
}

void checkBrightness()
{
    if (millis() - lastBrightnessCheck >= 10000) // check every 10 sec
    {
        return;
    }

    float lux = photocell.getCurrentLux();
    matrixBrightness = 255;

    if (lux <= 50)
    {
        matrixBrightness = map(lux, 0, 50, 20, 255);
    }

    matrix->setBrightness(matrixBrightness);

    lastBrightnessCheck = millis();
}

void checkServerIsOnline()
{
    if (!powerOn)
    {
        return;
    }

    if (millis() - lastMessageFromServer >= 60000) // more than one minute no message from server
    {
        matrix->clear();
        matrix->drawLine(0, 3, 31, 3, matrix->Color(255, 0, 0));
        matrix->show();
    }
}

static byte c1; // Last character buffer
byte utf8ascii(byte ascii)
{
    if (ascii < 128) // Standard ASCII-set 0..0x7F handling
    {
        c1 = 0;
        return (ascii);
    }
    // get previous input
    byte last = c1; // get last char
    c1 = ascii;     // remember actual character
    switch (last)   // conversion depending on first UTF8-character
    {
    case 0xC2:
        return (ascii)-34;
        break;
    case 0xC3:
        return (ascii | 0xC0) - 34;
        break;
    case 0x82:
        if (ascii == 0xAC)
            return (0xEA);
    }
    return (0);
}

String utf8ascii(String s)
{
    String r = "";
    char c;
    for (int i = 0; i < s.length(); i++)
    {
        c = utf8ascii(s.charAt(i));
        if (c != 0)
            r += c;
    }
    return r;
}

void utf8ascii(char *s)
{
    int k = 0;
    char c;
    for (int i = 0; i < strlen(s); i++)
    {
        c = utf8ascii(s[i]);
        if (c != 0)
            s[k++] = c;
    }
    s[k] = 0;
}

uint32_t Wheel(byte WheelPos, int pos)
{
    if (WheelPos < 85)
    {
        return matrix->Color((WheelPos * 3) - pos, (255 - WheelPos * 3) - pos, 0);
    }
    else if (WheelPos < 170)
    {
        WheelPos -= 85;
        return matrix->Color((255 - WheelPos * 3) - pos, 0, (WheelPos * 3) - pos);
    }
    else
    {
        WheelPos -= 170;
        return matrix->Color(0, (WheelPos * 3) - pos, (255 - WheelPos * 3) - pos);
    }
}
void hardwareAnimatedSearch(int typ, int x, int y)
{
    for (int i = 0; i < 4; i++)
    {
        matrix->clear();
        matrix->setTextColor(0xFFFF);

        if (typ == 0)
        {
            matrix->setCursor(7, 6);
            matrix->print("WiFi");
        }
        else if (typ == 1)
        {
            matrix->setCursor(1, 6);
            matrix->print("Server");
        }

        switch (i)
        {
        case 3:
            matrix->drawPixel(x, y, 0x22ff);
            matrix->drawPixel(x + 1, y + 1, 0x22ff);
            matrix->drawPixel(x + 2, y + 2, 0x22ff);
            matrix->drawPixel(x + 3, y + 3, 0x22ff);
            matrix->drawPixel(x + 2, y + 4, 0x22ff);
            matrix->drawPixel(x + 1, y + 5, 0x22ff);
            matrix->drawPixel(x, y + 6, 0x22ff);
        case 2:
            matrix->drawPixel(x - 1, y + 2, 0x22ff);
            matrix->drawPixel(x, y + 3, 0x22ff);
            matrix->drawPixel(x - 1, y + 4, 0x22ff);
        case 1:
            matrix->drawPixel(x - 3, y + 3, 0x22ff);
        case 0:
            break;
        }

        matrix->show();

        delay(100);
    }
}

void hardwareAnimatedCheck(int typ, int x, int y)
{
    int wifiCheckTime = millis();
    int wifiCheckPoints = 0;

    while (millis() - wifiCheckTime < 2000)
    {
        while (wifiCheckPoints < 7)
        {
            matrix->clear();

            switch (typ)
            {
            case 0:
                matrix->setCursor(7, 6);
                matrix->print("WiFi");
                break;
            case 1:
                matrix->setCursor(1, 6);
                matrix->print("Server");
                break;
            case 2:
                matrix->setCursor(7, 6);
                matrix->print("Temp");
                break;
            case 3:
                matrix->setCursor(3, 6);
                matrix->print("Audio");
                break;
            case 4:
                matrix->setCursor(3, 6);
                matrix->print("Gest.");
                break;
            case 5:
                matrix->setCursor(7, 6);
                matrix->print("LDR");
                break;
            }

            switch (wifiCheckPoints)
            {
            case 6:
                matrix->drawPixel(x, y, 0x07E0);
            case 5:
                matrix->drawPixel(x - 1, y + 1, 0x07E0);
            case 4:
                matrix->drawPixel(x - 2, y + 2, 0x07E0);
            case 3:
                matrix->drawPixel(x - 3, y + 3, 0x07E0);
            case 2:
                matrix->drawPixel(x - 4, y + 4, 0x07E0);
            case 1:
                matrix->drawPixel(x - 5, y + 3, 0x07E0);
            case 0:
                matrix->drawPixel(x - 6, y + 2, 0x07E0);
                break;
            }

            wifiCheckPoints++;

            matrix->show();

            delay(100);
        }
    }
}

void serverSearch(int rounds, int typ, int x, int y)
{
    matrix->clear();
    matrix->setTextColor(0xFFFF);
    matrix->setCursor(1, 6);
    matrix->print("Server");

    if (typ == 0)
    {
        switch (rounds)
        {
        case 3:
            matrix->drawPixel(x, y, 0x22ff);
            matrix->drawPixel(x + 1, y + 1, 0x22ff);
            matrix->drawPixel(x + 2, y + 2, 0x22ff);
            matrix->drawPixel(x + 3, y + 3, 0x22ff);
            matrix->drawPixel(x + 2, y + 4, 0x22ff);
            matrix->drawPixel(x + 1, y + 5, 0x22ff);
            matrix->drawPixel(x, y + 6, 0x22ff);
        case 2:
            matrix->drawPixel(x - 1, y + 2, 0x22ff);
            matrix->drawPixel(x, y + 3, 0x22ff);
            matrix->drawPixel(x - 1, y + 4, 0x22ff);
        case 1:
            matrix->drawPixel(x - 3, y + 3, 0x22ff);
        case 0:
            break;
        }
    }
    else if (typ == 1)
    {

        switch (rounds)
        {
        case 12:
            //matrix->drawPixel(x+3, y+2, 0x22ff);
            matrix->drawPixel(x + 3, y + 3, 0x22ff);
            //matrix->drawPixel(x+3, y+4, 0x22ff);
            matrix->drawPixel(x + 3, y + 5, 0x22ff);
            //matrix->drawPixel(x+3, y+6, 0x22ff);
        case 11:
            matrix->drawPixel(x + 2, y + 2, 0x22ff);
            matrix->drawPixel(x + 2, y + 3, 0x22ff);
            matrix->drawPixel(x + 2, y + 4, 0x22ff);
            matrix->drawPixel(x + 2, y + 5, 0x22ff);
            matrix->drawPixel(x + 2, y + 6, 0x22ff);
        case 10:
            matrix->drawPixel(x + 1, y + 3, 0x22ff);
            matrix->drawPixel(x + 1, y + 4, 0x22ff);
            matrix->drawPixel(x + 1, y + 5, 0x22ff);
        case 9:
            matrix->drawPixel(x, y + 4, 0x22ff);
        case 8:
            matrix->drawPixel(x - 1, y + 4, 0x22ff);
        case 7:
            matrix->drawPixel(x - 2, y + 4, 0x22ff);
        case 6:
            matrix->drawPixel(x - 3, y + 4, 0x22ff);
        case 5:
            matrix->drawPixel(x - 3, y + 5, 0x22ff);
        case 4:
            matrix->drawPixel(x - 3, y + 6, 0x22ff);
        case 3:
            matrix->drawPixel(x - 3, y + 7, 0x22ff);
        case 2:
            matrix->drawPixel(x - 4, y + 7, 0x22ff);
        case 1:
            matrix->drawPixel(x - 5, y + 7, 0x22ff);
        case 0:
            break;
        }
    }

    matrix->show();
}

void flashProgress(unsigned int progress, unsigned int total)
{
    matrix->clear();
    matrix->setBrightness(100);

    long num = 32 * 8 * progress / total;

    for (unsigned char y = 0; y < 8; y++)
    {
        for (unsigned char x = 0; x < 32; x++)
        {
            if (num-- > 0)
                matrix->drawPixel(x, 8 - y - 1, Wheel((num * 16) & 255, 0));
        }
    }

    matrix->setCursor(0, 6);
    matrix->setTextColor(matrix->Color(255, 255, 255));
    matrix->print("FLASHING");
    matrix->show();
}
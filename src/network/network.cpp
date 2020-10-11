#include "network.h"

#include <WiFi.h>
namespace Network
{
    WiFiClient client;
    const char *host = "192.168.43.1";
    const uint16_t port = 1234;

    void beginTask(){
        xTaskCreate(sendTask,           //任务函数
    		               "task1",         //这个参数没有什么作用，仅作为任务的描述
			                2048,            //任务栈的大小
			               NULL,         //传给任务函数的参数
			                2,              //优先级，数字越大优先级越高
			               NULL);
    }
    extern double temperature;
    void sendTask(void *pvPar)
    {
        uint64_t lastMillis = millis();
        while (1)
        {
            if (millis() - lastMillis > 80)
            {
                lastMillis = millis();
                uint8_t sbuf[3];
                short a = temperature*100;
                short b = 1;
                short c = 2;
                sbuf[0] = 't';
                sbuf[1] = a >> 8;
                sbuf[2] = a;
                // sbuf[3] = b >> 8;
                // sbuf[4] = b;
                // sbuf[5] = c >> 8;
                // sbuf[6] = c;
                // sbuf[7] = d>>8;
                // sbuf[8] = d;
                client.write(sbuf, 3);
            }
            // printf("I'm %s\r\n",(char *)pvPar);
            //使用此延时API可以将任务转入阻塞态，期间CPU继续运行其它任务
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
} // namespace Network
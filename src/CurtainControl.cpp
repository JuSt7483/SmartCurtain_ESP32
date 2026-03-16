#include <Arduino.h>
#include "CurtainControl.h"
#include "Config.h"

CurtainState currentStatus = CLOSED;

void initCurtain() {
    // Тут будут пины драйвера мотора
    Serial.println("[Module] Логика шторы инициализирована");
}

void openCurtain() {
    if (currentStatus != OPENED) {
        Serial.println("[Motor] КРУЧУ ВПЕРЕД: Открываю штору...");
        // Имитация процесса
        currentStatus = MOVING;
        delay(1000); 
        currentStatus = OPENED;
        Serial.println("[Motor] СТОП: Штора открыта.");
    }
}

void closeCurtain() {
    if (currentStatus != CLOSED) {
        Serial.println("[Motor] КРУЧУ НАЗАД: Закрываю штору...");
        currentStatus = MOVING;
        delay(1000);
        currentStatus = CLOSED;
        Serial.println("[Motor] СТОП: Штора закрыта.");
    }
}

CurtainState getCurtainStatus() {
    return currentStatus;
}
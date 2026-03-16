#ifndef CURTAIN_CONTROL_H
#define CURTAIN_CONTROL_H

#include <Arduino.h>

// Состояния шторы
enum CurtainState { CLOSED, OPENED, MOVING };

void initCurtain();         // Инициализация пинов
void openCurtain();         // Команда на открытие
void closeCurtain();        // Команда на закрытие
CurtainState getCurtainStatus(); // Геттер состояния

#endif
#include <Arduino.h>

namespace {

constexpr uint32_t kPlantStepMs = 40;
constexpr float kWaterFlowPerStep = 0.001F;
constexpr float kHeaterPower = 0.04F;
constexpr float kThermalLoss = 0.00002F;

constexpr float kLevelLow = 0.10F;
constexpr float kLevelHigh = 0.90F;
constexpr float kHeaterOnTemperature = 70.0F;
constexpr float kHeaterOffTemperature = 85.0F;

struct PlantState {
  float tankLevel[2] = {0.05F, 0.03F};
  float temperature[2] = {20.0F, 20.0F};
  bool valveOpen[3] = {false, false, false};
  bool heaterOn = false;
  bool levelHigh[2] = {false, false};
  bool levelLow[2] = {false, false};
  float dischargedVolume = 0.0F;
};

PlantState plant;
SemaphoreHandle_t plantMutex = nullptr;
uint32_t lastPlantUpdateMs = 0;

float clampLevel(float value) {
  if (value < 0.0F) {
    return 0.0F;
  }
  if (value > 1.0F) {
    return 1.0F;
  }
  return value;
}

void updateLevelSensors() {
  for (size_t tank = 0; tank < 2; ++tank) {
    plant.levelLow[tank] = plant.tankLevel[tank] >= kLevelLow;
    plant.levelHigh[tank] = plant.tankLevel[tank] >= kLevelHigh;
  }
}

void updatePlant() {
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastPlantUpdateMs) < kPlantStepMs) {
    return;
  }
  lastPlantUpdateMs = now;

  if (plant.valveOpen[0]) {
    plant.tankLevel[0] = clampLevel(plant.tankLevel[0] + kWaterFlowPerStep);
  }

  if (plant.valveOpen[1] && plant.tankLevel[0] > plant.tankLevel[1]) {
    const float requestedTransfer = kWaterFlowPerStep * 0.5F;
    const float transfer = min(requestedTransfer, plant.tankLevel[0]);
    const float previousTank2Level = plant.tankLevel[1];
    const float resultingTank2Level = previousTank2Level + transfer;

    if (resultingTank2Level > 0.0F) {
      plant.temperature[1] =
          ((previousTank2Level * plant.temperature[1]) +
           (transfer * plant.temperature[0])) /
          resultingTank2Level;
    }

    plant.tankLevel[0] = clampLevel(plant.tankLevel[0] - transfer);
    plant.tankLevel[1] = clampLevel(resultingTank2Level);
  }

  if (plant.valveOpen[2]) {
    const float discharge = min(kWaterFlowPerStep, plant.tankLevel[1]);
    plant.tankLevel[1] = clampLevel(plant.tankLevel[1] - discharge);
    plant.dischargedVolume += discharge;
  }

  if (plant.heaterOn) {
    plant.temperature[1] += kHeaterPower / (plant.tankLevel[1] + 0.1F);
  }
  plant.temperature[1] -=
      kThermalLoss * (plant.temperature[1] - plant.temperature[0]);

  updateLevelSensors();
}

void printPlantState() {
  Serial.printf(
      "tank1=%.3f tank2=%.3f temp1=%.2f temp2=%.2f "
      "valves=[%d,%d,%d] heater=%d discharged=%.3f\n",
      plant.tankLevel[0], plant.tankLevel[1], plant.temperature[0],
      plant.temperature[1], plant.valveOpen[0], plant.valveOpen[1],
      plant.valveOpen[2], plant.heaterOn, plant.dischargedVolume);
}

void plantUpdateTask(void *) {
  for (;;) {
    xSemaphoreTake(plantMutex, portMAX_DELAY);
    updatePlant();
    xSemaphoreGive(plantMutex);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void telemetryTask(void *) {
  for (;;) {
    xSemaphoreTake(plantMutex, portMAX_DELAY);
    printPlantState();
    xSemaphoreGive(plantMutex);
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void heaterControlTask(void *) {
  for (;;) {
    xSemaphoreTake(plantMutex, portMAX_DELAY);
    if (plant.temperature[1] <= kHeaterOnTemperature) {
      plant.heaterOn = true;
    } else if (plant.temperature[1] >= kHeaterOffTemperature) {
      plant.heaterOn = false;
    }
    xSemaphoreGive(plantMutex);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void inletControlTask(void *) {
  for (;;) {
    xSemaphoreTake(plantMutex, portMAX_DELAY);
    if (!plant.levelLow[0]) {
      plant.valveOpen[0] = true;
    } else if (plant.levelHigh[0]) {
      plant.valveOpen[0] = false;
    }
    xSemaphoreGive(plantMutex);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void transferControlTask(void *) {
  for (;;) {
    xSemaphoreTake(plantMutex, portMAX_DELAY);
    if (!plant.levelLow[1]) {
      plant.valveOpen[1] = true;
    } else if (plant.levelHigh[1]) {
      plant.valveOpen[1] = false;
    }
    xSemaphoreGive(plantMutex);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void outletControlTask(void *) {
  for (;;) {
    xSemaphoreTake(plantMutex, portMAX_DELAY);
    plant.valveOpen[2] =
        plant.levelLow[1] && plant.temperature[1] > kHeaterOnTemperature;
    xSemaphoreGive(plantMutex);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

bool createTask(TaskFunction_t task, const char *name, UBaseType_t priority) {
  return xTaskCreate(task, name, 2048, nullptr, priority, nullptr) == pdPASS;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  plantMutex = xSemaphoreCreateMutex();

  if (plantMutex == nullptr ||
      !createTask(plantUpdateTask, "plant-update", 2) ||
      !createTask(telemetryTask, "telemetry", 1) ||
      !createTask(heaterControlTask, "heater", 1) ||
      !createTask(inletControlTask, "inlet-valve", 1) ||
      !createTask(transferControlTask, "transfer-valve", 1) ||
      !createTask(outletControlTask, "outlet-valve", 1)) {
    Serial.println("Falha ao inicializar recursos do FreeRTOS.");
    for (;;) {
      delay(1000);
    }
  }
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}

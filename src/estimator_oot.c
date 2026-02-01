/**
 * Out-of-tree estimator for TinyMPC controller
 * This provides a simple pass-through estimator that uses the complementary estimator
 */

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "queue.h"

#include "stabilizer.h"
#include "estimator.h"
#include "sensfusion6.h"
#include "position_estimator.h"
#include "sensors.h"
#include "stabilizer_types.h"

static Axis3f gyro;
static Axis3f acc;
static baro_t baro;

#define ATTITUDE_UPDATE_RATE RATE_250_HZ
#define ATTITUDE_UPDATE_DT 1.0/ATTITUDE_UPDATE_RATE

void estimatorOutOfTreeInit(void)
{
  sensfusion6Init();
}

bool estimatorOutOfTreeTest(void)
{
  bool pass = true;
  pass &= sensfusion6Test();
  return pass;
}

void estimatorOutOfTree(state_t *state, const stabilizerStep_t stabilizerStep)
{
  // Pull the latest sensors values
  measurement_t m;
  while (estimatorDequeue(&m)) {
    switch (m.type)
    {
    case MeasurementTypeGyroscope:
      gyro = m.data.gyroscope.gyro;
      break;
    case MeasurementTypeAcceleration:
      acc = m.data.acceleration.acc;
      break;
    case MeasurementTypeBarometer:
      baro = m.data.barometer.baro;
      break;
    default:
      break;
    }
  }

  // Update filter
  if (RATE_DO_EXECUTE(ATTITUDE_UPDATE_RATE, stabilizerStep)) {
    sensfusion6UpdateQ(gyro.x, gyro.y, gyro.z,
                        acc.x, acc.y, acc.z,
                        ATTITUDE_UPDATE_DT);

    // Save attitude
    sensfusion6GetEulerRPY(&state->attitude.roll, &state->attitude.pitch, &state->attitude.yaw);

    // Save quaternion
    sensfusion6GetQuaternion(
      &state->attitudeQuaternion.x,
      &state->attitudeQuaternion.y,
      &state->attitudeQuaternion.z,
      &state->attitudeQuaternion.w);

    // Set acc
    state->acc.x = acc.x;
    state->acc.y = acc.y;
    state->acc.z = acc.z;
  }
}

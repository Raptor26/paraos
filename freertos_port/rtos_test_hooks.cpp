#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "interfaces/irtos_attr.h"
#include "interfaces/irtos_check.h"

#if (configUSE_MALLOC_FAILED_HOOK == 1)
extern "C" __ICORE_ATTR_WEAK void vApplicationMallocFailedHook(void) {
  __icore_checkLOOP();
}
#endif

#if (configUSE_IDLE_HOOK == 1)
#include <stdlib.h>
extern "C" __ICORE_ATTR_WEAK void vApplicationIdleHook(void) { exit(0); }
#endif

#if (configUSE_TICK_HOOK == 1)
extern "C" __ICORE_ATTR_WEAK void vApplicationTickHook(void) {}
#endif

#if (configUSE_DAEMON_TASK_STARTUP_HOOK == 1)
extern "C" __ICORE_ATTR_WEAK void vApplicationDaemonTaskStartupHook(void) {}
#endif

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
extern "C" void vApplicationStackOverflowHook(
    TaskHandle_t xTask, char *pcTaskName) {
  /* Check pcTaskName for the name of the offending task,
   * or pxCurrentTCB if pcTaskName has itself been corrupted. */
  (void)xTask;
  (void)pcTaskName;
}
#endif

#if (configSUPPORT_STATIC_ALLOCATION == 1)

static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

extern "C" __ICORE_ATTR_WEAK void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize) {
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

extern "C" __ICORE_ATTR_WEAK void vApplicationGetTimerTaskMemory(
    StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer,
    uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif /* #if (configSUPPORT_STATIC_ALLOCATION == 1) */
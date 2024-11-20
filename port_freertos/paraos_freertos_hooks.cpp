/// @file paraos_freertos_hooks.cpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
/// @author VyhodcevEgor <vyhodcev@internet.ru>
///
/// @copyright (c) 2024 Stilsoft
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "paraos_attr.h"
#include "paraos_check.h"
#include "paraos_utils.hpp"
#include "task.h"

#if (configUSE_MALLOC_FAILED_HOOK == 1)
extern "C" PARAOS_ATTR_WEAK void vApplicationMallocFailedHook(void) {
  PARAOS_CHECK_LOOP();
}
#endif

#if (configUSE_IDLE_HOOK == 1)
extern "C" PARAOS_ATTR_WEAK void vApplicationIdleHook(void) {
  using namespace paraos;
  if (freertos_idle_fnc_ptr) {
    freertos_idle_fnc_ptr();
  }
}
#endif

#if (configUSE_TICK_HOOK == 1)
extern "C" PARAOS_ATTR_WEAK void vApplicationTickHook(void) {}
#endif

#if (configUSE_DAEMON_TASK_STARTUP_HOOK == 1)
extern "C" PARAOS_ATTR_WEAK void vApplicationDaemonTaskStartupHook(void) {}
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

extern "C" PARAOS_ATTR_WEAK void vApplicationGetIdleTaskMemory(
    StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
    uint32_t *pulIdleTaskStackSize) {
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

extern "C" PARAOS_ATTR_WEAK void vApplicationGetTimerTaskMemory(
    StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer,
    uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif /* #if (configSUPPORT_STATIC_ALLOCATION == 1) */

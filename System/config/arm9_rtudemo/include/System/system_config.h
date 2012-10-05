#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#define DEBUG
#define INFO

#define RTUDEMO_UPDATING

#define RUNTIME_UPDATING
#define RTU_POINTER_TRACING

#define HAS_BINARY_REGISTER

#if defined(LINKER) || defined(TASK_MANAGER)
#undef DEBUG
#undef INFO
#endif

#endif /* SYSTEM_CONFIG_H */ 

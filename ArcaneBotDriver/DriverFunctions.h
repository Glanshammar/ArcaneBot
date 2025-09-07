#pragma once

#include <ntifs.h>
#include "ArcaneShared.h"

VOID AsyncWorkRoutine(PDEVICE_OBJECT DeviceObject, PVOID Context);
VOID ClickAt(RuneClient client, LONG32 x, LONG32 y);

typedef struct _ASYNC_CTX {
    PIRP Irp;                   // The IRP to complete later
    PIO_WORKITEM WorkItem;      // The work item
    ArcaneData Data;            // Copy of the input data
} ASYNC_CTX, * PASYNC_CTX;
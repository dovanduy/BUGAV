/*++
    Sample NDIS Lightweight filter driver
--*/

#include "precomp.h"

#define __FILENUMBER    'PNPF'

// This directive puts the DriverEntry function into the INIT segment of the
// driver.  To conserve memory, the code will be discarded when the driver's
// DriverEntry function returns.  You can declare other functions used only
// during initialization here.
#pragma NDIS_INIT_FUNCTION(DriverEntry)

// Global variables
NDIS_HANDLE         FilterDriverHandle; // NDIS handle for filter driver
NDIS_HANDLE         FilterDriverObject;
NDIS_HANDLE         NdisFilterDeviceHandle = NULL;
PDEVICE_OBJECT      NdisDeviceObject = NULL;

FILTER_LOCK         FilterListLock;
LIST_ENTRY          FilterModuleList;

NDIS_FILTER_PARTIAL_CHARACTERISTICS DefaultChars = {
{ 0, 0, 0},
      0,
      FilterSendNetBufferLists,
      FilterSendNetBufferListsComplete,
      NULL,
      FilterReceiveNetBufferLists,
      FilterReturnNetBufferLists
};


_Use_decl_annotations_
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT      DriverObject,
    PUNICODE_STRING     RegistryPath
)
/*++

Routine Description:

    First entry point to be called, when this driver is loaded.
    Register with NDIS as a filter driver and create a device
    for communication with user-mode.

Arguments:

    DriverObject - pointer to the system's driver object structure
                   for this driver

    RegistryPath - system's registry path for this driver

Return Value:

    STATUS_SUCCESS if all initialization is successful, STATUS_XXX
    error code if not.

--*/
{
    NDIS_STATUS Status;
    NDIS_FILTER_DRIVER_CHARACTERISTICS      FChars;
    NDIS_STRING ServiceName = RTL_CONSTANT_STRING(FILTER_SERVICE_NAME);
    NDIS_STRING UniqueName = RTL_CONSTANT_STRING(FILTER_UNIQUE_NAME);
    NDIS_STRING FriendlyName = RTL_CONSTANT_STRING(FILTER_FRIENDLY_NAME);
    BOOLEAN bFalse = FALSE;

    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("NDIS \t!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    DbgPrint("NDIS \t===>DriverEntry...\n");

    FilterDriverObject = DriverObject;

    do {
        NdisZeroMemory(&FChars, sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS));
        FChars.Header.Type = NDIS_OBJECT_TYPE_FILTER_DRIVER_CHARACTERISTICS;
        FChars.Header.Size = sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS);
#if NDIS_SUPPORT_NDIS61
        FChars.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_2;
#else
        FChars.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_1;
#endif

        FChars.MajorNdisVersion = FILTER_MAJOR_NDIS_VERSION;
        FChars.MinorNdisVersion = FILTER_MINOR_NDIS_VERSION;
        FChars.MajorDriverVersion = 1;
        FChars.MinorDriverVersion = 0;
        FChars.Flags = 0;

        FChars.FriendlyName = FriendlyName;
        FChars.UniqueName = UniqueName;
        FChars.ServiceName = ServiceName;

        //
        // TODO: Most handlers are optional, however, this sample includes them
        // all for illustrative purposes.  If you do not need a particular 
        // handler, set it to NULL and NDIS will more efficiently pass the
        // operation through on your behalf.
        //
        FChars.SetOptionsHandler = FilterRegisterOptions;
        FChars.AttachHandler = FilterAttach;
        FChars.DetachHandler = FilterDetach;
        FChars.RestartHandler = FilterRestart;
        FChars.PauseHandler = FilterPause;
        FChars.SetFilterModuleOptionsHandler = FilterSetModuleOptions;
        FChars.OidRequestHandler = FilterOidRequest;
        FChars.OidRequestCompleteHandler = FilterOidRequestComplete;
        FChars.CancelOidRequestHandler = FilterCancelOidRequest;

        FChars.SendNetBufferListsHandler = FilterSendNetBufferLists;
        FChars.ReturnNetBufferListsHandler = FilterReturnNetBufferLists;
        FChars.SendNetBufferListsCompleteHandler = FilterSendNetBufferListsComplete;
        FChars.ReceiveNetBufferListsHandler = FilterReceiveNetBufferLists;
        FChars.DevicePnPEventNotifyHandler = FilterDevicePnPEventNotify;
        FChars.NetPnPEventHandler = FilterNetPnPEvent;
        FChars.StatusHandler = FilterStatus;
        FChars.CancelSendNetBufferListsHandler = FilterCancelSendNetBufferLists;

        DriverObject->DriverUnload = FilterUnload;

        FilterDriverHandle = NULL;

        // Read Net Rules
        ndisReadCfg();

        // Initialize spin locks
        FILTER_INIT_LOCK(&FilterListLock);

        InitializeListHead(&FilterModuleList);

        Status = NdisFRegisterFilterDriver(DriverObject,
            (NDIS_HANDLE)FilterDriverObject,
            &FChars,
            &FilterDriverHandle);
        if (Status != NDIS_STATUS_SUCCESS) {
            DbgPrint("NDIS \tRegister filter driver failed.\n");
            break;
        }

        Status = FilterRegisterDevice();

        if (Status != NDIS_STATUS_SUCCESS) {
            NdisFDeregisterFilterDriver(FilterDriverHandle);
            FILTER_FREE_LOCK(&FilterListLock);
            DbgPrint("NDIS \tRegister device for the filter driver failed.\n");
            break;
        }


    } while (bFalse);


    DbgPrint("NDIS \t<===DriverEntry, Status = %8x\n", Status);
    return Status;

}

_Use_decl_annotations_
NDIS_STATUS
FilterRegisterOptions(
    NDIS_HANDLE  NdisFilterDriverHandle,
    NDIS_HANDLE  FilterDriverContext
)
/*++

Routine Description:

    Register optional handlers with NDIS.  This sample does not happen to
    have any optional handlers to register, so this routine does nothing
    and could simply have been omitted.  However, for illustrative purposes,
    it is presented here.

Arguments:

    NdisFilterDriverHandle - pointer the driver handle received from
                             NdisFRegisterFilterDriver

    FilterDriverContext    - pointer to our context passed into
                             NdisFRegisterFilterDriver

Return Value:

    NDIS_STATUS_SUCCESS

--*/
{
    DbgPrint("NDIS \t===>FilterRegisterOptions\n");

    ASSERT(NdisFilterDriverHandle == FilterDriverHandle);
    ASSERT(FilterDriverContext == (NDIS_HANDLE)FilterDriverObject);

    if ((NdisFilterDriverHandle != (NDIS_HANDLE)FilterDriverHandle) ||
        (FilterDriverContext != (NDIS_HANDLE)FilterDriverObject)) {
        return NDIS_STATUS_INVALID_PARAMETER;
    }

    DbgPrint("NDIS \t<===FilterRegisterOptions\n");

    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
NDIS_STATUS
FilterAttach(
    NDIS_HANDLE                     NdisFilterHandle,
    NDIS_HANDLE                     FilterDriverContext,
    PNDIS_FILTER_ATTACH_PARAMETERS  AttachParameters
)
/*++

Routine Description:

    Filter attach routine.
    Create filter's context, allocate NetBufferLists and NetBuffer pools and any
    other resources, and read configuration if needed.

Arguments:

    NdisFilterHandle - Specify a handle identifying this instance of the filter. FilterAttach
                       should save this handle. It is a required  parameter in subsequent calls
                       to NdisFxxx functions.
    FilterDriverContext - Filter driver context passed to NdisFRegisterFilterDriver.

    AttachParameters - attach parameters

Return Value:

    NDIS_STATUS_SUCCESS: FilterAttach successfully allocated and initialize data structures
                         for this filter instance.
    NDIS_STATUS_RESOURCES: FilterAttach failed due to insufficient resources.
    NDIS_STATUS_FAILURE: FilterAttach could not set up this instance of this filter and it has called
                         NdisWriteErrorLogEntry with parameters specifying the reason for failure.

N.B.:  FILTER can use NdisRegisterDeviceEx to create a device, so the upper
    layer can send Irps to the filter.

--*/
{
    PMS_FILTER              pFilter = NULL;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    NDIS_FILTER_ATTRIBUTES  FilterAttributes;
    ULONG                   Size;
    BOOLEAN               bFalse = FALSE;

    DbgPrint("NDIS \t===>FilterAttach: NdisFilterHandle %p\n", NdisFilterHandle);

    do {
        ASSERT(FilterDriverContext == (NDIS_HANDLE)FilterDriverObject);
        if (FilterDriverContext != (NDIS_HANDLE)FilterDriverObject) {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        // Verify the media type is supported.  This is a last resort; the
        // the filter should never have been bound to an unsupported miniport
        // to begin with.  If this driver is marked as a Mandatory filter (which
        // is the default for this sample; see the INF file), failing to attach 
        // here will leave the network adapter in an unusable state.
        //
        // Your setup/install code should not bind the filter to unsupported
        // media types.
        if ((AttachParameters->MiniportMediaType != NdisMedium802_3)
            && (AttachParameters->MiniportMediaType != NdisMediumWan)
            && (AttachParameters->MiniportMediaType != NdisMediumWirelessWan)) {
            DbgPrint("NDIS \tUnsupported media type.\n");

            Status = NDIS_STATUS_INVALID_PARAMETER;
            break;
        }

        Size = sizeof(MS_FILTER) +
            AttachParameters->FilterModuleGuidName->Length +
            AttachParameters->BaseMiniportInstanceName->Length +
            AttachParameters->BaseMiniportName->Length;

        pFilter = (PMS_FILTER)FILTER_ALLOC_MEM(NdisFilterHandle, Size);
        if (pFilter == NULL) {
            DbgPrint("NDIS \tFailed to allocate context structure.\n");
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pFilter, sizeof(MS_FILTER));

        pFilter->FilterModuleName.Length = pFilter->FilterModuleName.MaximumLength = AttachParameters->FilterModuleGuidName->Length;
        pFilter->FilterModuleName.Buffer = (PWSTR)((PUCHAR)pFilter + sizeof(MS_FILTER));
        NdisMoveMemory(pFilter->FilterModuleName.Buffer,
            AttachParameters->FilterModuleGuidName->Buffer,
            pFilter->FilterModuleName.Length);



        pFilter->MiniportFriendlyName.Length = pFilter->MiniportFriendlyName.MaximumLength = AttachParameters->BaseMiniportInstanceName->Length;
        pFilter->MiniportFriendlyName.Buffer = (PWSTR)((PUCHAR)pFilter->FilterModuleName.Buffer + pFilter->FilterModuleName.Length);
        NdisMoveMemory(pFilter->MiniportFriendlyName.Buffer,
            AttachParameters->BaseMiniportInstanceName->Buffer,
            pFilter->MiniportFriendlyName.Length);


        pFilter->MiniportName.Length = pFilter->MiniportName.MaximumLength = AttachParameters->BaseMiniportName->Length;
        pFilter->MiniportName.Buffer = (PWSTR)((PUCHAR)pFilter->MiniportFriendlyName.Buffer +
            pFilter->MiniportFriendlyName.Length);
        NdisMoveMemory(pFilter->MiniportName.Buffer,
            AttachParameters->BaseMiniportName->Buffer,
            pFilter->MiniportName.Length);

        pFilter->MiniportIfIndex = AttachParameters->BaseMiniportIfIndex;
        //
        // The filter should initialize TrackReceives and TrackSends properly. For this
        // driver, since its default characteristic has both a send and a receive handler,
        // these fields are initialized to TRUE.
        //
        pFilter->TrackReceives = TRUE;
        pFilter->TrackSends = TRUE;
        pFilter->FilterHandle = NdisFilterHandle;


        NdisZeroMemory(&FilterAttributes, sizeof(NDIS_FILTER_ATTRIBUTES));
        FilterAttributes.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
        FilterAttributes.Header.Size = sizeof(NDIS_FILTER_ATTRIBUTES);
        FilterAttributes.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
        FilterAttributes.Flags = 0;

        NDIS_DECLARE_FILTER_MODULE_CONTEXT(MS_FILTER);
        Status = NdisFSetAttributes(NdisFilterHandle,
            pFilter,
            &FilterAttributes);
        if (Status != NDIS_STATUS_SUCCESS) {
            DbgPrint("NDIS \tFailed to set attributes.\n");
            break;
        }


        pFilter->State = FilterPaused;

        FILTER_ACQUIRE_LOCK(&FilterListLock, bFalse);
        InsertHeadList(&FilterModuleList, &pFilter->FilterModuleLink);
        FILTER_RELEASE_LOCK(&FilterListLock, bFalse);

    } while (bFalse);

    if (Status != NDIS_STATUS_SUCCESS) {
        if (pFilter != NULL) {
            FILTER_FREE_MEM(pFilter);
        }
    }

    DbgPrint("NDIS \t<===FilterAttach:    Status %x\n", Status);
    return Status;
}

_Use_decl_annotations_
NDIS_STATUS
FilterPause(
    NDIS_HANDLE                     FilterModuleContext,
    PNDIS_FILTER_PAUSE_PARAMETERS   PauseParameters
)
/*++

Routine Description:

    Filter pause routine.
    Complete all the outstanding sends and queued sends,
    wait for all the outstanding recvs to be returned
    and return all the queued receives.

Arguments:

    FilterModuleContext - pointer to the filter context stucture
    PauseParameters     - additional information about the pause

Return Value:

    NDIS_STATUS_SUCCESS if filter pauses successfully, NDIS_STATUS_PENDING
    if not.  No other return value is allowed (pause must succeed, eventually).

N.B.: When the filter is in Pausing state, it can still process OID requests,
    complete sending, and returning packets to NDIS, and also indicate status.
    After this function completes, the filter must not attempt to send or
    receive packets, but it may still process OID requests and status
    indications.

--*/
{
    PMS_FILTER          pFilter = (PMS_FILTER)(FilterModuleContext);
    NDIS_STATUS         Status;
    BOOLEAN               bFalse = FALSE;

    UNREFERENCED_PARAMETER(PauseParameters);

    DbgPrint("NDIS \t===>NDISLWF FilterPause: FilterInstance %p\n", FilterModuleContext);

    //
    // Set the flag that the filter is going to pause
    //
    FILTER_ASSERT(pFilter->State == FilterRunning);

    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);
    pFilter->State = FilterPausing;
    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);

    //
    // Do whatever work is required to bring the filter into the Paused state.
    //
    // If you have diverted and queued any send or receive NBLs, return them 
    // now.
    //
    // If you send or receive original NBLs, stop doing that and wait for your
    // NBLs to return to you now.
    //


    Status = NDIS_STATUS_SUCCESS;

    pFilter->State = FilterPaused;

    DbgPrint("NDIS \t<===FilterPause:  Status %x\n", Status);
    return Status;
}

_Use_decl_annotations_
NDIS_STATUS
FilterRestart(
    NDIS_HANDLE                     FilterModuleContext,
    PNDIS_FILTER_RESTART_PARAMETERS RestartParameters
)
/*++

Routine Description:

    Filter restart routine.
    Start the datapath - begin sending and receiving NBLs.

Arguments:

    FilterModuleContext - pointer to the filter context stucture.
    RestartParameters   - additional information about the restart operation.

Return Value:

    NDIS_STATUS_SUCCESS: if filter restarts successfully
    NDIS_STATUS_XXX: Otherwise.

--*/
{
    NDIS_STATUS     Status;
    PMS_FILTER      pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_HANDLE     ConfigurationHandle = NULL;


    PNDIS_RESTART_GENERAL_ATTRIBUTES NdisGeneralAttributes;
    PNDIS_RESTART_ATTRIBUTES         NdisRestartAttributes;
    NDIS_CONFIGURATION_OBJECT        ConfigObject;

    DbgPrint("NDIS \t===>FilterRestart:   FilterModuleContext %p\n", FilterModuleContext);

    FILTER_ASSERT(pFilter->State == FilterPaused);

    ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
    ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
    ConfigObject.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);
    ConfigObject.NdisHandle = FilterDriverHandle;
    ConfigObject.Flags = 0;

    Status = NdisOpenConfigurationEx(&ConfigObject, &ConfigurationHandle);
    if (Status != NDIS_STATUS_SUCCESS) {
        //
        // Filter driver can choose to fail the restart if it cannot open the configuration
        //

#if 0
        //
        // The code is here just to demonstrate how to call NDIS to write an 
        // event to the eventlog.
        //
        PWCHAR              ErrorString = L"Ndislwf";

        DEBUGP(DL_WARN, "FilterRestart: Cannot open configuration.\n");
        NdisWriteEventLogEntry(FilterDriverObject,
            EVENT_NDIS_DRIVER_FAILURE,
            0,
            1,
            &ErrorString,
            sizeof(Status),
            &Status);
#endif

    }

    //
    // This sample doesn't actually do anything with the configuration handle;
    // it is opened here for illustrative purposes.  If you do not need to
    // read configuration, you may omit the code manipulating the 
    // ConfigurationHandle.
    //

    if (Status == NDIS_STATUS_SUCCESS) {
        NdisCloseConfiguration(ConfigurationHandle);
    }

    NdisRestartAttributes = RestartParameters->RestartAttributes;

    //
    // If NdisRestartAttributes is not NULL, then the filter can modify generic 
    // attributes and add new media specific info attributes at the end. 
    // Otherwise, if NdisRestartAttributes is NULL, the filter should not try to 
    // modify/add attributes.
    //
    if (NdisRestartAttributes != NULL) {
        PNDIS_RESTART_ATTRIBUTES   NextAttributes;

        ASSERT(NdisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES);

        NdisGeneralAttributes = (PNDIS_RESTART_GENERAL_ATTRIBUTES)NdisRestartAttributes->Data;

        //
        // Check to see if we need to change any attributes. For example, the
        // driver can change the current MAC address here. Or the driver can add
        // media specific info attributes.
        //
        NdisGeneralAttributes->LookaheadSize = 128;

        //
        // Check each attribute to see whether the filter needs to modify it.
        //
        NextAttributes = NdisRestartAttributes->Next;

        while (NextAttributes != NULL) {
            //
            // If somehow the filter needs to change a attributes which requires more space then
            // the current attributes:
            // 1. Remove the attribute from the Attributes list:
            //    TempAttributes = NextAttributes;
            //    NextAttributes = NextAttributes->Next;
            // 2. Free the memory for the current attributes: NdisFreeMemory(TempAttributes, 0 , 0);
            // 3. Dynamically allocate the memory for the new attributes by calling
            //    NdisAllocateMemoryWithTagPriority:
            //    NewAttributes = NdisAllocateMemoryWithTagPriority(Handle, size, Priority);
            // 4. Fill in the new attribute
            // 5. NewAttributes->Next = NextAttributes;
            // 6. NextAttributes = NewAttributes; // Just to make the next statement work.
            //
            NextAttributes = NextAttributes->Next;
        }

        //
        // Add a new attributes at the end
        // 1. Dynamically allocate the memory for the new attributes by calling
        //    NdisAllocateMemoryWithTagPriority.
        // 2. Fill in the new attribute
        // 3. NextAttributes->Next = NewAttributes;
        // 4. NewAttributes->Next = NULL;



    }

    //
    // If everything is OK, set the filter in running state.
    //
    pFilter->State = FilterRunning; // when successful


    Status = NDIS_STATUS_SUCCESS;


    // Ensure the state is Paused if restart failed.
    if (Status != NDIS_STATUS_SUCCESS) {
        pFilter->State = FilterPaused;
    }


    DbgPrint("NDIS \t<===FilterRestart:  FilterModuleContext %p, Status %x\n", FilterModuleContext, Status);
    return Status;
}


_Use_decl_annotations_
VOID
FilterDetach(
    NDIS_HANDLE     FilterModuleContext
)
/*++

Routine Description:

    Filter detach routine.
    This is a required function that will deallocate all the resources allocated during
    FilterAttach. NDIS calls FilterAttach to remove a filter instance from a filter stack.

Arguments:

    FilterModuleContext - pointer to the filter context area.

Return Value:
    None.

NOTE: Called at PASSIVE_LEVEL and the filter is in paused state

--*/
{
    PMS_FILTER                  pFilter = (PMS_FILTER)FilterModuleContext;
    BOOLEAN                      bFalse = FALSE;


    DbgPrint("NDIS \t===>FilterDetach:    FilterInstance %p\n", FilterModuleContext);


    // Filter must be in paused state
    FILTER_ASSERT(pFilter->State == FilterPaused);


    // Detach must not fail, so do not put any code here that can possibly fail.

    // Free filter instance name if allocated.
    if (pFilter->FilterName.Buffer != NULL) {
        FILTER_FREE_MEM(pFilter->FilterName.Buffer);
    }


    FILTER_ACQUIRE_LOCK(&FilterListLock, bFalse);
    RemoveEntryList(&pFilter->FilterModuleLink);
    FILTER_RELEASE_LOCK(&FilterListLock, bFalse);

    // Free the memory allocated
    FILTER_FREE_MEM(pFilter);

    DbgPrint("NDIS \t<===FilterDetach Successfully\n");
    return;
}

_Use_decl_annotations_
VOID
FilterUnload(
    PDRIVER_OBJECT      DriverObject
)
/*++

Routine Description:

    Filter driver's unload routine.
    Deregister the driver from NDIS.

Arguments:

    DriverObject - pointer to the system's driver object structure
                   for this driver

Return Value:

    NONE

--*/
{
#if DBG
    BOOLEAN               bFalse = FALSE;
#endif

    UNREFERENCED_PARAMETER(DriverObject);

    DbgPrint("NDIS \t===>FilterUnload\n");

    // Should free the filter context list
    FilterDeregisterDevice();
    NdisFDeregisterFilterDriver(FilterDriverHandle);

#if DBG
    FILTER_ACQUIRE_LOCK(&FilterListLock, bFalse);
    ASSERT(IsListEmpty(&FilterModuleList));

    FILTER_RELEASE_LOCK(&FilterListLock, bFalse);

#endif

    FILTER_FREE_LOCK(&FilterListLock);

    DbgPrint("NDIS \t<===FilterUnload\n");

    return;

}

_Use_decl_annotations_
NDIS_STATUS
FilterOidRequest(
    NDIS_HANDLE         FilterModuleContext,
    PNDIS_OID_REQUEST   Request
)
/*++

Routine Description:

    Request handler
    Handle requests from upper layers

Arguments:

    FilterModuleContext   - our filter
    Request               - the request passed down


Return Value:

     NDIS_STATUS_SUCCESS
     NDIS_STATUS_PENDING
     NDIS_STATUS_XXX

NOTE: Called at <= DISPATCH_LEVEL  (unlike a miniport's MiniportOidRequest)

--*/
{
    PMS_FILTER              pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_STATUS             Status;
    PNDIS_OID_REQUEST       ClonedRequest = NULL;
    BOOLEAN                 bSubmitted = FALSE;
    PFILTER_REQUEST_CONTEXT Context;
    BOOLEAN                 bFalse = FALSE;


    //DbgPrint("NDIS \t===>FilterOidRequest: Request %p.\n", Request);

    //
    // Most of the time, a filter will clone the OID request and pass down
    // the clone.  When the clone completes, the filter completes the original
    // OID request.
    //
    // If your filter needs to modify a specific request, it can modify the
    // request before or after sending down the cloned request.  Or, it can
    // complete the original request on its own without sending down any
    // clone at all.
    //
    // If your filter driver does not need to modify any OID requests, then
    // you may simply omit this routine entirely; NDIS will pass OID requests
    // down on your behalf.  This is more efficient than implementing a 
    // routine that does nothing but clone all requests, as in the sample here.
    //

    do {
        Status = NdisAllocateCloneOidRequest(pFilter->FilterHandle,
            Request,
            FILTER_TAG,
            &ClonedRequest);
        if (Status != NDIS_STATUS_SUCCESS) {
            DbgPrint("NDIS \tFilerOidRequest: Cannot Clone Request\n");
            break;
        }

        Context = (PFILTER_REQUEST_CONTEXT)(&ClonedRequest->SourceReserved[0]);
        *Context = Request;

        bSubmitted = TRUE;

        // Use same request ID
        ClonedRequest->RequestId = Request->RequestId;

        pFilter->PendingOidRequest = ClonedRequest;


        Status = NdisFOidRequest(pFilter->FilterHandle, ClonedRequest);

        if (Status != NDIS_STATUS_PENDING) {


            FilterOidRequestComplete(pFilter, ClonedRequest, Status);
            Status = NDIS_STATUS_PENDING;
        }


    } while (bFalse);

    if (bSubmitted == FALSE) {
        switch (Request->RequestType) {
        case NdisRequestMethod:
            Request->DATA.METHOD_INFORMATION.BytesRead = 0;
            Request->DATA.METHOD_INFORMATION.BytesNeeded = 0;
            Request->DATA.METHOD_INFORMATION.BytesWritten = 0;
            break;

        case NdisRequestSetInformation:
            Request->DATA.SET_INFORMATION.BytesRead = 0;
            Request->DATA.SET_INFORMATION.BytesNeeded = 0;
            break;

        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
        default:
            Request->DATA.QUERY_INFORMATION.BytesWritten = 0;
            Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
            break;
        }

    }
    //DbgPrint("NDIS \t<===FilterOidRequest: Status %8x.\n", Status);

    return Status;

}

_Use_decl_annotations_
VOID
FilterCancelOidRequest(
    NDIS_HANDLE             FilterModuleContext,
    PVOID                   RequestId
)
/*++

Routine Description:

    Cancels an OID request

    If your filter driver does not intercept and hold onto any OID requests,
    then you do not need to implement this routine.  You may simply omit it.
    Furthermore, if the filter only holds onto OID requests so it can pass
    down a clone (the most common case) the filter does not need to implement
    this routine; NDIS will then automatically request that the lower-level
    filter/miniport cancel your cloned OID.

    Most filters do not need to implement this routine.

Arguments:

    FilterModuleContext   - our filter
    RequestId             - identifies the request(s) to cancel

--*/
{
    PMS_FILTER                          pFilter = (PMS_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   Request = NULL;
    PFILTER_REQUEST_CONTEXT             Context;
    PNDIS_OID_REQUEST                   OriginalRequest = NULL;
    BOOLEAN                             bFalse = FALSE;

    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);

    Request = pFilter->PendingOidRequest;

    if (Request != NULL) {
        Context = (PFILTER_REQUEST_CONTEXT)(&Request->SourceReserved[0]);

        OriginalRequest = (*Context);
    }

    if ((OriginalRequest != NULL) && (OriginalRequest->RequestId == RequestId)) {
        FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);

        NdisFCancelOidRequest(pFilter->FilterHandle, RequestId);
    } else {
        FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);
    }
}

_Use_decl_annotations_
VOID
FilterOidRequestComplete(
    NDIS_HANDLE         FilterModuleContext,
    PNDIS_OID_REQUEST   Request,
    NDIS_STATUS         Status
)
/*++

Routine Description:

    Notification that an OID request has been completed

    If this filter sends a request down to a lower layer, and the request is
    pended, the FilterOidRequestComplete routine is invoked when the request
    is complete.  Most requests we've sent are simply clones of requests
    received from a higher layer; all we need to do is complete the original
    higher request.

    However, if this filter driver sends original requests down, it must not
    attempt to complete a pending request to the higher layer.

Arguments:

    FilterModuleContext   - our filter context area
    NdisRequest           - the completed request
    Status                - completion status

--*/
{
    PMS_FILTER                          pFilter = (PMS_FILTER)FilterModuleContext;
    PNDIS_OID_REQUEST                   OriginalRequest;
    PFILTER_REQUEST_CONTEXT             Context;
    BOOLEAN                             bFalse = FALSE;

    //DbgPrint("NDIS \t===>FilterOidRequestComplete, Request %p.\n", Request);

    Context = (PFILTER_REQUEST_CONTEXT)(&Request->SourceReserved[0]);
    OriginalRequest = (*Context);

    // This is an internal request
    if (OriginalRequest == NULL) {
        filterInternalRequestComplete(pFilter, Request, Status);
        return;
    }


    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);

    ASSERT(pFilter->PendingOidRequest == Request);
    pFilter->PendingOidRequest = NULL;

    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);

    // Copy the information from the returned request to the original request
    switch (Request->RequestType) {
    case NdisRequestMethod:
        OriginalRequest->DATA.METHOD_INFORMATION.OutputBufferLength = Request->DATA.METHOD_INFORMATION.OutputBufferLength;
        OriginalRequest->DATA.METHOD_INFORMATION.BytesRead = Request->DATA.METHOD_INFORMATION.BytesRead;
        OriginalRequest->DATA.METHOD_INFORMATION.BytesNeeded = Request->DATA.METHOD_INFORMATION.BytesNeeded;
        OriginalRequest->DATA.METHOD_INFORMATION.BytesWritten = Request->DATA.METHOD_INFORMATION.BytesWritten;
        break;

    case NdisRequestSetInformation:
        OriginalRequest->DATA.SET_INFORMATION.BytesRead = Request->DATA.SET_INFORMATION.BytesRead;
        OriginalRequest->DATA.SET_INFORMATION.BytesNeeded = Request->DATA.SET_INFORMATION.BytesNeeded;
        break;

    case NdisRequestQueryInformation:
    case NdisRequestQueryStatistics:
    default:
        OriginalRequest->DATA.QUERY_INFORMATION.BytesWritten = Request->DATA.QUERY_INFORMATION.BytesWritten;
        OriginalRequest->DATA.QUERY_INFORMATION.BytesNeeded = Request->DATA.QUERY_INFORMATION.BytesNeeded;
        break;
    }


    (*Context) = NULL;

    NdisFreeCloneOidRequest(pFilter->FilterHandle, Request);
    NdisFOidRequestComplete(pFilter->FilterHandle, OriginalRequest, Status);
    //DbgPrint("NDIS \t<===FilterOidRequestComplete.\n");
}


_Use_decl_annotations_
VOID
FilterStatus(
    NDIS_HANDLE             FilterModuleContext,
    PNDIS_STATUS_INDICATION StatusIndication
)
/*++

Routine Description:

    Status indication handler

Arguments:

    FilterModuleContext     - our filter context
    StatusIndication        - the status being indicated

NOTE: called at <= DISPATCH_LEVEL

  FILTER driver may call NdisFIndicateStatus to generate a status indication to
  all higher layer modules.

--*/
{
    PMS_FILTER              pFilter = (PMS_FILTER)FilterModuleContext;
#if DBG
    BOOLEAN                  bFalse = FALSE;
#endif

    DbgPrint("NDIS \t===>FilterStatus, IndicateStatus = %8x.\n", StatusIndication->StatusCode);


    //
    // The filter may do processing on the status indication here, including
    // intercepting and dropping it entirely.  However, the sample does nothing
    // with status indications except pass them up to the higher layer.  It is 
    // more efficient to omit the FilterStatus handler entirely if it does 
    // nothing, but it is included in this sample for illustrative purposes.
    //

#if DBG
    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);
    ASSERT(pFilter->bIndicating == FALSE);
    pFilter->bIndicating = TRUE;
    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);
#endif // DBG

    NdisFIndicateStatus(pFilter->FilterHandle, StatusIndication);

#if DBG
    FILTER_ACQUIRE_LOCK(&pFilter->Lock, bFalse);
    ASSERT(pFilter->bIndicating == TRUE);
    pFilter->bIndicating = FALSE;
    FILTER_RELEASE_LOCK(&pFilter->Lock, bFalse);
#endif // DBG

    DbgPrint("NDIS \t<===FilterStatus.\n");

}

_Use_decl_annotations_
VOID
FilterDevicePnPEventNotify(
    NDIS_HANDLE             FilterModuleContext,
    PNET_DEVICE_PNP_EVENT   NetDevicePnPEvent
)
/*++

Routine Description:

    Device PNP event handler

Arguments:

    FilterModuleContext         - our filter context
    NetDevicePnPEvent           - a Device PnP event

NOTE: called at PASSIVE_LEVEL

--*/
{
    PMS_FILTER             pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_DEVICE_PNP_EVENT  DevicePnPEvent = NetDevicePnPEvent->DevicePnPEvent;
#if DBG
    BOOLEAN                bFalse = FALSE;
#endif

    DbgPrint("NDIS \t===>FilterDevicePnPEventNotify: NetPnPEvent = %p.\n", NetDevicePnPEvent);

    // The filter may do processing on the event here, including intercepting
    // and dropping it entirely.  However, the sample does nothing with Device
    // PNP events, except pass them down to the next lower* layer.  It is more
    // efficient to omit the FilterDevicePnPEventNotify handler entirely if it
    // does nothing, but it is included in this sample for illustrative purposes.
    //
    // * Trivia: Device PNP events percolate DOWN the stack, instead of upwards
    // like status indications and Net PNP events.  So the next layer is the
    // LOWER layer.

    switch (DevicePnPEvent) {

    case NdisDevicePnPEventQueryRemoved:
    case NdisDevicePnPEventRemoved:
    case NdisDevicePnPEventSurpriseRemoved:
    case NdisDevicePnPEventQueryStopped:
    case NdisDevicePnPEventStopped:
    case NdisDevicePnPEventPowerProfileChanged:
    case NdisDevicePnPEventFilterListChanged:

        break;

    default:
        DbgPrint("NDIS \tFilterDevicePnPEventNotify: Invalid event.\n");
        FILTER_ASSERT(bFalse);

        break;
    }

    NdisFDevicePnPEventNotify(pFilter->FilterHandle, NetDevicePnPEvent);

    DbgPrint("NDIS \t<===FilterDevicePnPEventNotify\n");

}

_Use_decl_annotations_
NDIS_STATUS
FilterNetPnPEvent(
    NDIS_HANDLE              FilterModuleContext,
    PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification
)
/*++

Routine Description:

    Net PNP event handler

Arguments:

    FilterModuleContext         - our filter context
    NetPnPEventNotification     - a Net PnP event

NOTE: called at PASSIVE_LEVEL

--*/
{
    PMS_FILTER                pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_STATUS               Status = NDIS_STATUS_SUCCESS;

    //
    // The filter may do processing on the event here, including intercepting 
    // and dropping it entirely.  However, the sample does nothing with Net PNP
    // events, except pass them up to the next higher layer.  It is more
    // efficient to omit the FilterNetPnPEvent handler entirely if it does
    // nothing, but it is included in this sample for illustrative purposes.
    //

    Status = NdisFNetPnPEvent(pFilter->FilterHandle, NetPnPEventNotification);

    return Status;
}

_Use_decl_annotations_
NDIS_STATUS
FilterSetModuleOptions(
    NDIS_HANDLE             FilterModuleContext
)
/*++

Routine Description:

    This function set the optional handlers for the filter

Arguments:

    FilterModuleContext: The FilterModuleContext given to NdisFSetAttributes

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_FAILURE

--*/
{
    PMS_FILTER                               pFilter = (PMS_FILTER)FilterModuleContext;
    NDIS_FILTER_PARTIAL_CHARACTERISTICS      OptionalHandlers;
    NDIS_STATUS                              Status = NDIS_STATUS_SUCCESS;
    BOOLEAN                                  bFalse = FALSE;

    //
    // Demonstrate how to change send/receive handlers at runtime.
    //
    if (bFalse) {
        UINT      i;


        pFilter->CallsRestart++;

        i = pFilter->CallsRestart % 8;

        pFilter->TrackReceives = TRUE;
        pFilter->TrackSends = TRUE;

        NdisMoveMemory(&OptionalHandlers, &DefaultChars, sizeof(OptionalHandlers));
        OptionalHandlers.Header.Type = NDIS_OBJECT_TYPE_FILTER_PARTIAL_CHARACTERISTICS;
        OptionalHandlers.Header.Size = sizeof(OptionalHandlers);
        switch (i) {

        case 0:
            OptionalHandlers.ReceiveNetBufferListsHandler = NULL;
            pFilter->TrackReceives = FALSE;
            break;

        case 1:

            OptionalHandlers.ReturnNetBufferListsHandler = NULL;
            pFilter->TrackReceives = FALSE;
            break;

        case 2:
            OptionalHandlers.SendNetBufferListsHandler = NULL;
            pFilter->TrackSends = FALSE;
            break;

        case 3:
            OptionalHandlers.SendNetBufferListsCompleteHandler = NULL;
            pFilter->TrackSends = FALSE;
            break;

        case 4:
            OptionalHandlers.ReceiveNetBufferListsHandler = NULL;
            OptionalHandlers.ReturnNetBufferListsHandler = NULL;
            break;

        case 5:
            OptionalHandlers.SendNetBufferListsHandler = NULL;
            OptionalHandlers.SendNetBufferListsCompleteHandler = NULL;
            break;

        case 6:

            OptionalHandlers.ReceiveNetBufferListsHandler = NULL;
            OptionalHandlers.ReturnNetBufferListsHandler = NULL;
            OptionalHandlers.SendNetBufferListsHandler = NULL;
            OptionalHandlers.SendNetBufferListsCompleteHandler = NULL;
            break;

        case 7:
            break;
        }
        Status = NdisSetOptionalHandlers(pFilter->FilterHandle, (PNDIS_DRIVER_OPTIONAL_HANDLERS)&OptionalHandlers);
    }
    return Status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NDIS_STATUS
filterDoInternalRequest(
    _In_ PMS_FILTER                   FilterModuleContext,
    _In_ NDIS_REQUEST_TYPE            RequestType,
    _In_ NDIS_OID                     Oid,
    _Inout_updates_bytes_to_(InformationBufferLength, *pBytesProcessed)
    PVOID                        InformationBuffer,
    _In_ ULONG                        InformationBufferLength,
    _In_opt_ ULONG                    OutputBufferLength,
    _In_ ULONG                        MethodId,
    _Out_ PULONG                      pBytesProcessed
)
/*++

Routine Description:

    Utility routine that forms and sends an NDIS_OID_REQUEST to the
    miniport, waits for it to complete, and returns status
    to the caller.

    NOTE: this assumes that the calling routine ensures validity
    of the filter handle until this returns.

Arguments:

    FilterModuleContext - pointer to our filter module context
    RequestType - NdisRequest[Set|Query|method]Information
    Oid - the object being set/queried
    InformationBuffer - data for the request
    InformationBufferLength - length of the above
    OutputBufferLength  - valid only for method request
    MethodId - valid only for method request
    pBytesProcessed - place to return bytes read/written

Return Value:

    Status of the set/query request

--*/
{
    FILTER_REQUEST              FilterRequest;
    PNDIS_OID_REQUEST           NdisRequest = &FilterRequest.Request;
    NDIS_STATUS                 Status;
    BOOLEAN                     bFalse;


    bFalse = FALSE;
    *pBytesProcessed = 0;
    NdisZeroMemory(NdisRequest, sizeof(NDIS_OID_REQUEST));

    NdisInitializeEvent(&FilterRequest.ReqEvent);

    NdisRequest->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
    NdisRequest->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
    NdisRequest->Header.Size = sizeof(NDIS_OID_REQUEST);
    NdisRequest->RequestType = RequestType;

    switch (RequestType) {
    case NdisRequestQueryInformation:
        NdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
            InformationBuffer;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
            InformationBufferLength;
        break;

    case NdisRequestSetInformation:
        NdisRequest->DATA.SET_INFORMATION.Oid = Oid;
        NdisRequest->DATA.SET_INFORMATION.InformationBuffer =
            InformationBuffer;
        NdisRequest->DATA.SET_INFORMATION.InformationBufferLength =
            InformationBufferLength;
        break;

    case NdisRequestMethod:
        NdisRequest->DATA.METHOD_INFORMATION.Oid = Oid;
        NdisRequest->DATA.METHOD_INFORMATION.MethodId = MethodId;
        NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer =
            InformationBuffer;
        NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength =
            InformationBufferLength;
        NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength = OutputBufferLength;
        break;



    default:
        FILTER_ASSERT(bFalse);
        break;
    }

    NdisRequest->RequestId = (PVOID)FILTER_REQUEST_ID;

    Status = NdisFOidRequest(FilterModuleContext->FilterHandle,
        NdisRequest);


    if (Status == NDIS_STATUS_PENDING) {

        NdisWaitEvent(&FilterRequest.ReqEvent, 0);
        Status = FilterRequest.Status;
    }


    if (Status == NDIS_STATUS_SUCCESS) {
        if (RequestType == NdisRequestSetInformation) {
            *pBytesProcessed = NdisRequest->DATA.SET_INFORMATION.BytesRead;
        }

        if (RequestType == NdisRequestQueryInformation) {
            *pBytesProcessed = NdisRequest->DATA.QUERY_INFORMATION.BytesWritten;
        }

        if (RequestType == NdisRequestMethod) {
            *pBytesProcessed = NdisRequest->DATA.METHOD_INFORMATION.BytesWritten;
        }

        //
        // The driver below should set the correct value to BytesWritten
        // or BytesRead. But now, we just truncate the value to InformationBufferLength
        //
        if (RequestType == NdisRequestMethod) {
            if (*pBytesProcessed > OutputBufferLength) {
                *pBytesProcessed = OutputBufferLength;
            }
        } else {

            if (*pBytesProcessed > InformationBufferLength) {
                *pBytesProcessed = InformationBufferLength;
            }
        }
    }


    return Status;
}

VOID
filterInternalRequestComplete(
    _In_ NDIS_HANDLE                  FilterModuleContext,
    _In_ PNDIS_OID_REQUEST            NdisRequest,
    _In_ NDIS_STATUS                  Status
)
/*++

Routine Description:

    NDIS entry point indicating completion of a pended NDIS_OID_REQUEST.

Arguments:

    FilterModuleContext - pointer to filter module context
    NdisRequest - pointer to NDIS request
    Status - status of request completion

Return Value:

    None

--*/
{
    PFILTER_REQUEST              FilterRequest;


    UNREFERENCED_PARAMETER(FilterModuleContext);

    //
    //  Get at the request context.
    //
    FilterRequest = CONTAINING_RECORD(NdisRequest, FILTER_REQUEST, Request);

    //
    //  Save away the completion status.
    //
    FilterRequest->Status = Status;

    //
    //  Wake up the thread blocked for this request to complete.
    //
    NdisSetEvent(&FilterRequest->ReqEvent);
}


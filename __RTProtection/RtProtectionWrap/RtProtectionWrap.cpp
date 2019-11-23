#include "pch.h"

#include "RtProtectionWrap.h"

RtProtectionWrap::RtProtectionWrap() 
    : 
    ptr_RtProtectionCtrl(new RtProtectionCtrl()),
    ptr_RtProtectionInjectCtrl(new RtProtectionInjectCtrl())
{}

RtProtectionWrap::~RtProtectionWrap() { delete ptr_RtProtectionCtrl; }

VOID RtProtectionWrap::WRAP_RtProtectionDrv_LoadDriver() { loaded = ptr_RtProtectionCtrl->RtProtectionDrv_LoadDriver(); }

VOID RtProtectionWrap::WRAP_RtProtectionDrv_UnloadDriver() { 
    bool res = ptr_RtProtectionCtrl->RtProtectionDrv_UnloadDriver();
    if (res) {
        loaded = false;
    }
}

bool RtProtectionWrap::WRAP_RtProtectionDrv_NewProcMon() {
    NEWPROC_INFO proc_info = { 0 };
    bool res = ptr_RtProtectionCtrl->RtProtectionDrv_NewProcMon(&proc_info);
    if (res) {
        _ParentId = (int)proc_info.ParentId;
        _ProcessId= (int)proc_info.ProcessId;
        _Create= proc_info.Create;
    }
    return res;
}

VOID RtProtectionWrap::WRAP_InjectLib(int pid) {
    PWCHAR dll_to_inject = L"RtProtectionPayloadDll.dll";
    DWORD dll_data = 1000;
    ULONG data_sz = sizeof(DWORD);
    ptr_RtProtectionInjectCtrl->InjectLib((DWORD)pid, dll_to_inject, &dll_data, data_sz);
}

bool RtProtectionWrap::Get_loaded() { return loaded; }

int RtProtectionWrap::Get_ParentId() { return _ParentId; }

int RtProtectionWrap::Get_ProcessId() { return _ProcessId; }

int RtProtectionWrap::Get_Create() { return _Create; }


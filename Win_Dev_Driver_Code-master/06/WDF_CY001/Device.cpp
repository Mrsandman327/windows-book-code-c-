/*
	��Ȩ��CY001 WDF USB��������Ŀ  2009/9/1
	
	����ӵ�еĴ˷ݴ��뿽�����������ڸ���ѧϰ���κ���ҵ�����������;����ʹ�ã�������������
	�����δ���������벻Ҫ�������������ϴ�������Ŀ�������ѽ����뷢����������ά����

	���ߣ��������		���� 
		  ����������	����
		  AMD			�ĺ���

	���ڣ�2009/9/1

	�ļ���Device.c
	˵������ӡDebug��Ϣ
	��ʷ��
	������
*/
#ifdef __cplusplus
extern "C"{
#endif

#include "public.h"

#ifdef __cplusplus
}
#endif

#include "DrvClass.h"

// ��ҳ�����к������ǿɻ�ҳ�ģ������ڱ�������(PASSIVE_LEVEL)
#pragma code_seg("PAGE")

// ��ӿ��豸���ó�ʼ��Alterֵ
UCHAR MultiInterfaceSettings[MAX_INTERFACES] = {1, 1, 1, 1, 1, 1, 1, 1};

// USB���߽ӿ�GUID
// {B1A96A13-3DE0-4574-9B01-C08FEAB318D6}
static GUID USB_BUS_INTERFACE_USBDI_GUID1 =
{ 0xb1a96a13, 0x3de0, 0x4574, {0x9b, 0x1, 0xc0, 0x8f, 0xea, 0xb3, 0x18, 0xd6}};

// �á�GUID Generator����������һ��Ψһ��GUID��
static	GUID DeviceInterface = 
{0xdb713b3f, 0xea3f, 0x4d74, {0x89, 0x46, 0x0, 0x64, 0xdb, 0xa4, 0xfc, 0x6a}};

//
// �����൱��WDM�е�AddDevice������
// ����PNP�������׵���屻���õĻص���������Ҫ�����Ǵ����豸����
// �豸������ϵͳ����������ʽ�������ⲿ��ĳ������Ӳ���豸��
//NTSTATUS PnpAdd(IN WDFDRIVER  Driver, IN PWDFDEVICE_INIT  DeviceInit)
NTSTATUS DrvClass::PnpAdd(IN PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS status;
	WDFDEVICE device;
	int nInstance = 0;
	PDEVICE_CONTEXT devCtx = NULL;
	UNICODE_STRING DeviceName;
	UNICODE_STRING DosDeviceName;
	UNICODE_STRING RefString;
	WDFSTRING	   SymbolName;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDF_DEVICE_PNP_CAPABILITIES pnpCapabilities;

	WCHAR wcsDeviceName[] = L"\\Device\\CY001-0";		// �豸��
	WCHAR wcsDosDeviceName[] = L"\\DosDevices\\CY001-0";// ����������
	WCHAR wcsRefString[] = L"CY001-0";					// ����������
	int	  nLen = wcslen(wcsDeviceName);

	KDBG(DPFLTR_INFO_LEVEL, "[PnpAdd]");

	RtlInitUnicodeString(&DeviceName, wcsDeviceName) ;
	RtlInitUnicodeString(&DosDeviceName, wcsDosDeviceName);
	RtlInitUnicodeString(&RefString, wcsRefString);

	// ע��PNP��Power�ص�����
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
	pnpPowerCallbacks.EvtDevicePrepareHardware	= PnpPrepareHardware_sta; // ��ʼ��
	pnpPowerCallbacks.EvtDeviceReleaseHardware  = PnpReleaseHardware_sta; // ֹͣ
	pnpPowerCallbacks.EvtDeviceSurpriseRemoval	= PnpSurpriseRemove_sta;  // �쳣�Ƴ�
	pnpPowerCallbacks.EvtDeviceD0Entry	= PwrD0Entry_sta; // ����D0��Դ״̬������״̬����������β��롢���߻���
	pnpPowerCallbacks.EvtDeviceD0Exit	= PwrD0Exit_sta;  // �뿪D0��Դ״̬������״̬�����������߻��豸�Ƴ�
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	// ����д����Ļ��巽ʽ
	// Ĭ��ΪBuffered��ʽ���������ַ�ʽ��Direct��Neither��
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

	// �趨�豸�����鳤��
	// ���ڲ������sizeof(DEVICE_CONTEXT)��ṹ�峤��
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
	
	// ����֧�����8��ʵ���������������������PC��ʱ�����������Ǹ��貢��֧�֡�
	// ��ͬ���豸���󣬸��������Ƶ�β����0-7����𣬲�����β�������豸ID��
	// ����Ĵ����߼�Ϊ��ǰ�豸����Ѱ��һ������ID��
	for(nInstance = 0; nInstance < MAX_INSTANCE_NUMBER; nInstance++)
	{
		// �޸��豸ID
		wcsDeviceName[nLen-1] += nInstance;

		// ������������������ʧ�ܣ�ʧ�ܵ�ԭ�������������Ч�������Ѵ��ڵ�
		WdfDeviceInitAssignName(DeviceInit, &DeviceName);

		// ����WDF�豸
		// ���ܳɹ�����������ɹ���
		status = WdfDeviceCreate(&DeviceInit, &attributes, &device);  
		if(!NT_SUCCESS(status))
		{
			if(status == STATUS_OBJECT_NAME_COLLISION)// ���ֳ�ͻ
			{
				KDBG(DPFLTR_ERROR_LEVEL, "Already exist: %wZ", &DeviceName);
			}
			else
			{
				KDBG(DPFLTR_ERROR_LEVEL, "WdfDeviceCreate failed with status 0x%08x!!!", status);
				return status;
			}
		}else
		{
			KdPrint(("Device name: %wZ", &DeviceName));
			break;
		}
	}

	// ��ʧ�ܣ����������ӵĿ���������̫�ࣻ
	// ����ʹ��WinOBJ�鿴ϵͳ�е��豸���ơ�
	if (!NT_SUCCESS(status)) 
		return status;

	// �����������ӣ�Ӧ�ó�����ݷ������Ӳ鿴��ʹ���ں��豸��
	// ���˴������������⣬���ڸ��õķ�����ʹ��WdfDeviceCreateDeviceInterface�����豸�ӿڡ�
	// �豸�ӿ��ܱ�֤���ֲ����ͻ���������пɶ��ԣ����������Բ��÷���������ʽ��
	nLen = wcslen(wcsDosDeviceName);
	wcsDosDeviceName[nLen-1] += nInstance;
	
	status = WdfDeviceCreateSymbolicLink(device, &DosDeviceName);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "Failed to create symbol link: 0x%08X", status);
		return status;
	}

#if 0
	// WdfDeviceCreateDeviceInterface������һ�������ַ�����������㣻
	// ������ͬһ���ӿ����еĶ���豸�ӿ���������
	nLen = wcslen(wcsRefString);
	wcsRefString[nLen-1] += nInstance;

	status = WdfDeviceCreateDeviceInterface(device, 
			&DeviceInterface,	// �ӿ�GUID
			&RefString);		// Ӧ���ַ���

	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "Failed to create the device interface: 0x%08X", status);
		return status;
	}

	status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &string);
	if(NT_SUCCESS(status))
	{
		status = WdfDeviceRetrieveDeviceInterfaceString(device, &DeviceInterface, &RefString, string);
		if(status == STATUS_SUCCESS)
		{
			UNICODE_STRING name;
			WdfStringGetUnicodeString(string, &name);
			KDBG(DPFLTR_INFO_LEVEL, "Interface Name:%wZ", &name);
		}
	}
#endif

	// PNP���ԡ������豸�쳣�γ���ϵͳ���ᵯ�������
	WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCapabilities);
	pnpCapabilities.Removable = WdfTrue;
	pnpCapabilities.SurpriseRemovalOK = WdfTrue;

	WdfDeviceSetPnpCapabilities(device, &pnpCapabilities);

	// ��ʼ��������
	// GetDeviceContext��һ������ָ�룬�ɺ�WDF_DECLARE_CONTEXT_TYPE_WITH_NAME���塣
	// �ο�pulibc.h�е�˵����
	devCtx = GetDeviceContext(device);

	RtlZeroMemory(devCtx, sizeof(DEVICE_CONTEXT));

	status = CreateWDFQueues(device, devCtx);
	if(!NT_SUCCESS(status))
		return status;


	// ���汻ע�͵Ĵ��룬��ʾ��һ����WDM����Ա��WDF������Ƕ��WDM����ķ�����
	// ͨ����ȡFunction Device Object���豸ջ����һ���Device Object��
	// ���ǿ����ڵõ�Request�󣬴�Request����ȡ��IRP�ṹ���ƹ�WDK��ܶ����д�����
	// ��Request����ȡIRP�ĺ����ǣ�WdfRequestWdmGetIrp
#if 0
	PDEVICE_OBJECT pFunctionDevice = WdfDeviceWdmGetDeviceObject(device);// �����豸����
	WDFDEVICE DeviceStack = WdfDeviceGetIoTarget(device);
	PDEVICE_OBJECT pDeviceNext = WdfIoTargetWdmGetTargetDeviceObject(DeviceStack);// ��Attached���²�����֮�豸����
#endif

	return status;
}

NTSTATUS DrvClass::CreateWDFQueues(WDFDEVICE Device, PDEVICE_CONTEXT pContext)
{
}

// �˺���������WDM�е�PNP_MN_STOP_DEVICE���������豸�Ƴ�ʱ�����á�
// ��������������ʱ���豸�Դ��ڹ���״̬��
NTSTATUS DrvClass::PnpReleaseHardware(IN WDFDEVICE Device, IN WDFCMRESLIST ResourceListTranslated)
{
	NTSTATUS                             status;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS  configParams;
	PDEVICE_CONTEXT                      pDeviceContext;

	UNREFERENCED_PARAMETER(ResourceListTranslated);

	KDBG(DPFLTR_INFO_LEVEL, "[PnpReleaseHardware]");

	pDeviceContext = GetDeviceContext(Device);

	// ���PnpPrepareHardware����ʧ��,UsbDeviceΪ�գ�
	// ��ʱ��ֱ�ӷ��ؼ��ɡ�
	if (pDeviceContext->UsbDevice == NULL)
		return STATUS_SUCCESS;

	// ȡ��USB�豸������IO��������������ȡ������Pipe��IO������
	WdfIoTargetStop(WdfUsbTargetDeviceGetIoTarget(pDeviceContext->UsbDevice), WdfIoTargetCancelSentIo);

	// Deconfiguration���ߡ������á�
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_DECONFIG(&configParams);
	status = WdfUsbTargetDeviceSelectConfig(
		pDeviceContext->UsbDevice,
		WDF_NO_OBJECT_ATTRIBUTES, 
		&configParams);

	return STATUS_SUCCESS;
}

// �˺���������WDM�е�PNP_MN_START_DEVICE������������PnpAdd֮�󱻵��á�
// ��ʱPNP�������������֮���Ѿ���������Щϵͳ��Դ�������ǰ�豸��
// ����ResourceList��ResourceListTranslated��������Щϵͳ��Դ��
// ��������������ʱ���豸�Ѿ�������D0��Դ״̬��������ɺ��豸����ʽ���빤��״̬��
NTSTATUS DrvClass::PnpPrepareHardware(IN WDFDEVICE Device, IN WDFCMRESLIST ResourceList, IN WDFCMRESLIST ResourceListTranslated)
{
	NTSTATUS status;
	PDEVICE_CONTEXT devCtx = NULL; 
	ULONG ulNumRes = 0;
	ULONG ulIndex;
	CM_PARTIAL_RESOURCE_DESCRIPTOR*  pResDes = NULL;

	KDBG(DPFLTR_INFO_LEVEL, "[PnpPrepareHardware]");

	devCtx = GetDeviceContext(Device);

	// �����豸
	status = ConfigureUsbDevice(Device, devCtx);
	if(!NT_SUCCESS(status))
		return status;
	
	// ��ȡPipe���
	status = GetUsbPipes(devCtx);
	if(!NT_SUCCESS(status))
		return status;

	// ��ʼ��Դ���ԣ�
	status = InitPowerManagement();
	//if(!NT_SUCCESS(status))
	//	return status;

	// ��ȡUSB���������ӿڡ����߽ӿ��а������������ṩ�Ļص�������������Ϣ��
	// ���߽ӿ���ϵͳ����GUID��ʶ��
	status = WdfFdoQueryForInterface(
		Device,
		&USB_BUS_INTERFACE_USBDI_GUID1,		// ���߽ӿ�ID
		(PINTERFACE)&devCtx->busInterface,	// �������豸��������
		sizeof(USB_BUS_INTERFACE_USBDI_V1),
		1, NULL);

	// ���ýӿں������ж�USB�汾��
	if(NT_SUCCESS(status) && devCtx->busInterface.IsDeviceHighSpeed){
		devCtx->bIsDeviceHighSpeed = devCtx->busInterface.IsDeviceHighSpeed(devCtx->busInterface.BusContext);
		KDBG(DPFLTR_INFO_LEVEL, "USB 2.0");
	}else
		KDBG(DPFLTR_INFO_LEVEL, "USB 1.1");

	// ��ϵͳ��Դ�б������ǲ���ʵ���ԵĲ�����������ӡһЩ��Ϣ��
	// ������ȫ���԰��������Щ���붼ע�͵���
	ulNumRes = WdfCmResourceListGetCount(ResourceList);
	KDBG(DPFLTR_INFO_LEVEL, "ResourceListCount:%d\n", ulNumRes);

	// ���������������µ�ö����Щϵͳ��Դ������ӡ�����ǵ����������Ϣ��
	for(ulIndex = 0; ulIndex < ulNumRes; ulIndex++)
	{
		pResDes = WdfCmResourceListGetDescriptor(ResourceList, ulIndex);		
		if(!pResDes)continue; // ȡ��ʧ�ܣ���������һ��
		
		switch (pResDes->Type) 
		{
			case CmResourceTypeMemory:
				KDBG(DPFLTR_INFO_LEVEL, "System Resource��CmResourceTypeMemory\n");
				break;

			case CmResourceTypePort:
				KDBG(DPFLTR_INFO_LEVEL, "System Resource��CmResourceTypePort\n");
				break;

			case CmResourceTypeInterrupt:
				KDBG(DPFLTR_INFO_LEVEL, "System Resource��CmResourceTypeInterrupt\n");
				break;

			default:
				KDBG(DPFLTR_INFO_LEVEL, "System Resource��Others %d\n", pResDes->Type);
				break;
		}
	}

	return STATUS_SUCCESS;
}

// ��ʼ���ṹ��WDF_USB_INTERFACE_SETTING_PAIR��
// �������ö�ӿ��豸��
int  InitSettingPairs(
	IN WDFUSBDEVICE UsbDevice,					// �豸����
	OUT PWDF_USB_INTERFACE_SETTING_PAIR Pairs,  // �ṹ��ָ�롣
	IN ULONG NumSettings						// �ӿڸ���
)
{
	int i;

 	// ���֧��8���ӿڣ��Ѷ�����е���
	if(NumSettings > MAX_INTERFACES)
		NumSettings = MAX_INTERFACES;

	// ���ýӿ�
	for(i = 0; i < NumSettings; i++) {
		Pairs[i].m_hUsbInterface = WdfUsbTargetDeviceGetInterface(UsbDevice, i);// ���ýӿھ��
		Pairs[i].SettingIndex = MultiInterfaceSettings[i];					 // ���ýӿڿ�ѡֵ(Alternate Setting)
	}

	return NumSettings;
}

// �豸����
// ����WDF��ܣ��豸����ѡ��Ĭ��Ϊ1������ڶ������ѡ���Ҫ�л�ѡ��Ļ�����Ƚ��鷳��
// һ�ְ취�ǣ�ʹ�ó�ʼ���꣺WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_INTERFACES_DESCRIPTORS 
// ʹ������꣬��Ҫ���Ȼ�ȡ����������������ؽӿ���������
// ��һ�ְ취�ǣ�ʹ��WDM�������ȹ���һ������ѡ���URB��Ȼ��Ҫô�Լ�����IRP���͵�����������
// Ҫôʹ��WDF��������WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_URB��ʼ���ꡣ
// 
NTSTATUS DrvClass::ConfigureUsbDevice(WDFDEVICE Device, PDEVICE_CONTEXT DeviceContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS usbConfig;
	PWDF_USB_INTERFACE_SETTING_PAIR settingPairs;
	UCHAR numInterfaces;
	WDF_USB_INTERFACE_SELECT_SETTING_PARAMS  interfaceSelectSetting;

	KDBG(DPFLTR_INFO_LEVEL, "[ConfigureUsbDevice]");

	// ����Usb�豸����
	// USB�豸���������ǽ���USB��������㡣�󲿷ֵ�USB�ӿں�����������������еġ�
	// USB�豸���󱻴������������Լ�ά������ܱ�������������Ҳ����������
	status = WdfUsbTargetDeviceCreate(Device, WDF_NO_OBJECT_ATTRIBUTES, &DeviceContext->UsbDevice);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfUsbTargetDeviceCreate failed with status 0x%08x\n", status);
		return status;
	}

	// �ӿ�����
	// WDF�ṩ�˶��ֽӿ����õĳ�ʼ���꣬�ֱ���Ե�һ�ӿڡ���ӿڵ�USB�豸��
	// ��ʼ���껹�ṩ���ڶ�����ü�����л���;�������������������ġ�
	// ��ѡ��Ĭ�����õ�����£��豸���ý��ޱȼ򵥣��򵥵��������ĥ���ں˳���Ա����۾���
	// ��ΪWDM�ϰ��еĴ����߼�������ֻҪ�����о͹��ˡ�
	numInterfaces = WdfUsbTargetDeviceGetNumInterfaces(DeviceContext->UsbDevice);
	if(1 == numInterfaces)
	{
		KDBG(DPFLTR_INFO_LEVEL, "There is one interface.", status);
		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&usbConfig);
	}
	else// ��ӿ�
	{
		int nNum;
		KDBG(DPFLTR_INFO_LEVEL, "There are %d interfaces.", numInterfaces);
		settingPairs = (WDF_USB_INTERFACE_SETTING_PAIR*)ExAllocatePoolWithTag(PagedPool,
						sizeof(WDF_USB_INTERFACE_SETTING_PAIR) * numInterfaces, POOLTAG);

		if (settingPairs == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;

		nNum = InitSettingPairs(DeviceContext->UsbDevice, settingPairs, numInterfaces);
		WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&usbConfig, nNum, settingPairs);
	}

	status = WdfUsbTargetDeviceSelectConfig(DeviceContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &usbConfig);
	if(!NT_SUCCESS(status))
	{
		KDBG(DPFLTR_INFO_LEVEL, "WdfUsbTargetDeviceSelectConfig failed with status 0x%08x\n", status);
		return status;
	}

	// ����ӿ�
	if(1 == numInterfaces)
	{
		DeviceContext->m_hUsbInterface = usbConfig.Types.SingleInterface.ConfiguredUsbInterface;

		// ʹ��SINGLE_INTERFACE�ӿ����ú꣬�ӿڵ�AltSettingֵĬ��Ϊ0��
		// �������д�����ʾ������ֶ��޸�ĳ�ӿڵ�AltSettingֵ���˴�Ϊ1��.
		WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&interfaceSelectSetting, 1);
		status = WdfUsbInterfaceSelectSetting(DeviceContext->m_hUsbInterface, WDF_NO_OBJECT_ATTRIBUTES, &interfaceSelectSetting);
	}
	else
	{
		int i;
		DeviceContext->m_hUsbInterface = usbConfig.Types.MultiInterface.Pairs[0].m_hUsbInterface;
		for(i = 0; i < numInterfaces; i++)
			DeviceContext->MulInterfaces[i] = usbConfig.Types.MultiInterface.Pairs[i].m_hUsbInterface;
	}

	return status;
}

//
// �豸���úú󣬽ӿڡ��ܵ����Ѵ����ˡ�
NTSTATUS DrvClass::GetUsbPipes(PDEVICE_CONTEXT DeviceContext)
{
}

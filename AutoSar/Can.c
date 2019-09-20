/*
 * can.c
 *
 *  Created on: Sep 20, 2019
 *      Author: Sad MultiVerse
 */

/*
 *
 * Important Includes of TivaWare Libraries
 *
 */
#include "../TivaWare/stdint.h"
#include "../TivaWare/hw_can.h"
#include "../TivaWare/hw_ints.h"
#include "../TivaWare/hw_nvic.h"
#include "../TivaWare/hw_memmap.h"
#include "../TivaWare/hw_sysctl.h"
#include "../TivaWare/hw_types.h"

#include "../TivaWare/can.h"
#include "../TivaWare/interrupt.h"

/**************************************************/

#include "Can.h"
#include "CanIf_Cbk.h"
#include "Det.h"


/*

STATIC Can_HwType MsgObject [MAX_NO_OF_OBJECTS] ;
/*
 *
	but if init function did its job we can use it from the configurations itself
*/
STATIC Can_ConfigType * g_Config_Ptr ;
STATIC uint8 Can_DriverState =  CAN_INITIALIZED ;						/// assume that init function has initialized it !

uint8 HTH_Semaphore = 0 ;




Std_ReturnType Can_Write (

		Can_HwHandleType 	Hth,
		const Can_PduType* 	PduInfo
)
{

	Std_ReturnType returnVal = E_OK ;


#if (CanDevErrorDetect == FALSE)
	if (Can_DriverState == CAN_NOT_INITIALIZED)
	{
		//// call the DET function .. CAN_E_UNIT
		returnVal = E_NOT_OK ;
	}

	else if (Hth > 64)
	{
		//// call the DET .... CAN_E_PARAM_HANDLE
		returnVal = E_NOT_OK ;
	}
	else if (PduInfo == NULL_PTR)
		{
			//// call the DET .... CAN_E_PARAM_POINTER
			returnVal = E_NOT_OK ;
		}
	else if (PduInfo->length > 64)
		{
			//// call the DET .... CAN_E_PARAM_DATA_LENGTH
			returnVal = E_NOT_OK ;
		}
	else

#endif
	{

		uint8 index ;

		uint32_t 		uiBase ;
		uint32_t 		ui32ObjID ;
		tCANMsgObject *	psMsgObject ;
		tMsgObjType 	eMsgType ;


		if (HTH_Semaphore == 0 ) 					//// 0 : no one uses it at the moment !

		{
			///// start protecting your shared stuff man !
			HTH_Semaphore = 1 ;											//// acquire me

			/*
			 * choosing what can controller .. to detect what is the base address
			 */

			for (index = 0 ; index < MAX_NO_OF_OBJECTS ; index++ )
			{
				if (g_Config_Ptr->HardWareObject[index].CanObjectId == Hth )
				{
					switch (g_Config_Ptr->HardWareObject[index].CanContrlloerRef)
					{
						case 0 : uiBase = CAN0_BASE ; break ;
						case 1 : uiBase = CAN1_BASE ; break ;
					}
				}
			}

			/*
			 * Message Object Id
			 */

			ui32ObjID = Hth ;

			/*
			 * PDU .. message object itself
			 */
			psMsgObject->ui32MsgID = PduInfo->id ;
			psMsgObject->ui32MsgLen = PduInfo->length ;
			psMsgObject->pui8MsgData = PduInfo->sdu ;
			psMsgObject->ui32Flags = 0;									/// don't forget me
			psMsgObject->ui32MsgIDMask = 0 ;							// and me .. or you just can delete us from Tivaware itself, if you don't love us anymore !


			/*
			 * the type of the CAN_write () function is obviously Tx
			 */
			eMsgType = MSG_OBJ_TYPE_TX ;


			/////// stop protecting your stuff man !
			HTH_Semaphore = 0 ; 											//// release me

			/*
			 * check if the controller is busy
			 */

			if (HWREG(uiBase + CAN_O_IF1CRQ) & CAN_IF1CRQ_BUSY)
			{
				returnVal = CAN_BUSY ;
			}

			else {

				/*
				 *
				 * call the Tivaware function now !
				 */
				CANMessageSet( uiBase , ui32ObjID, psMsgObject, eMsgType  );
				returnVal = E_OK ;

			}

		}
		else															/////// CAN Is busy
		{
			returnVal = CAN_BUSY ;
		}



	}

	return returnVal ;
}




/*---------------------------------------------------------------------
Function Name:  <Can_MainFunction_Mode>
Service ID:     <0x0c>
Description:
- Scheduled function.
- This function performs the polling of CAN controller mode transitions.
- it implements the polling of CAN status register flags to detect transition of CAN Controller state.
----------------------------------------------------------------------*/
void Can_MainFunction_Mode( void )
{
    uint8 ControllerIndex;
    uint32 ui32Base;

    for (ControllerIndex = 0 ; ControllerIndex < CONFIGURED_CHANNELS ; ControllerIndex++  )
    {
        switch (ControllerIndex)
        {
        case 0: ui32Base = CAN0_BASE ;break;
        case 1: ui32Base = CAN1_BASE ;break;
        }

        if (HWREG(ui32Base +CAN_O_CTR) & CAN_CTL_INIT)
        {
            if (ControllerMode == CAN_CS_SLEEP)
            {
                CanIf_ControllerModeIndication(ControllerIndex,CAN_CS_SLEEP);
            }
            else
            {
                CanIf_ControllerModeIndication(ControllerIndex,CAN_CS_STOPPED);
            }
        }
        else
        {
            CanIf_ControllerModeIndication(ControllerIndex,CAN_CS_STARTED);
        }
    }

}


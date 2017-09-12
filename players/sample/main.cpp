
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h> //catch ctrl+c
#include <unistd.h> //usleep
#include <pthread.h> //threading
#include <ctype.h>
#include <string.h>

#include "../../common/MT_ALSA_daemon_message_defs.h"
#include "../../../../../../MTCommon/Sources/MTAL/MTAL_IPC.h"


pthread_t thread;
bool exiting = false;

int MTPTP_IPC_IOCTL_Callback(void* cb_user, uint32_t ui32MsgId, void const * pui8InBuffer, uint32_t ui32InBufferSize, void* pui8OutBuffer, uint32_t* pui32OutBufferSize)
{
    *pui32OutBufferSize = 0;
    MT_ALSA_d_msg_id msg_id = (MT_ALSA_d_msg_id)ui32MsgId;
    
    switch (msg_id)
    {
        case MT_ALSA_d_On_NADAC_Added:
        {
            //MT_ALSA_d_T_device_id device_id; 
            std::cout << "NADAC found on the network" << std::endl;
            break;
        }
        case MT_ALSA_d_On_NADAC_Removed:    
            std::cout << "NADAC leave the network" << std::endl;
            break;
        case MT_ALSA_d_On_NADAC_PlayerSource_Lost:
            std::cout << "NADAC player source lost" << std::endl;
            break;
        case MT_ALSA_d_On_NADAC_PlayerSource_Selected:
            std::cout << "NADAC player source selected" << std::endl;
            break;
        default:
            std::cout << "Unexpected message receive. msg id = " << ui32MsgId << std::endl;
            break;
    }
    return 0;
}

void* run(void*)
{
    uintptr_t pptrHandle;
    int ret = MTAL_IPC_init(1, 2, &MTPTP_IPC_IOCTL_Callback, NULL/*cb_user*/, &pptrHandle);
    if (ret < 0)
    {
        std::cout << "MTAL_IPC_init return MsgId = " << ret << std::endl;
        return NULL;
    }
    
    std::cout << std::endl;
    std::cout << "***** NADAC player control API sample menu *****" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << "* get the current NADAC target id" << std::endl;  
    std::cout << ". set a NADAC target id" << std::endl;    
    std::cout << "/ list the registered NADAC" << std::endl;    
    std::cout << "+ select the player source on the targeted NADAC" << std::endl;   
    std::cout << "1 Display play icon on the targeted NADAC" << std::endl;
    std::cout << "2 Display pause icon on the targeted NADAC" << std::endl;
    std::cout << "3 Display none icon on the targeted NADAC" << std::endl;  
    std::cout << "T Display a custom text on the targeted NADAC" << std::endl;  
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl; 
    std::cout << "Press enter to exit..." << std::endl;
    
    MT_ALSA_d_error_code err_code = MT_ALSA_d_E_SUCCESSFULL;
    uint32_t out_size;
    int c;
    do
    {
        c =  getchar();
        //printf("%c = 0x%x\n", c, c);
        switch (c)
        {
            case '*':
            {
                MT_ALSA_d_T_device_id device_id;
                out_size = sizeof(MT_ALSA_d_T_device_id);
                
                err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                    pptrHandle, MT_ALSA_d_GetTarget_NADAC, 
                    NULL, 0, &device_id, &out_size);
                    
                std::cout << "Get NADAC target ";
                switch (err_code)
                {
                    case MT_ALSA_d_E_SUCCESSFULL:
                        std::cout << "sucessfully completed. Target = " << device_id.device_id << std::endl;
                        break;
                    case MT_ALSA_d_E_NOT_FOUND:
                        std::cout << "failed. No target found." << std::endl;
                        break;
                    default:
                        std::cout << "failed. Unexpected error." << err_code << std::endl;
                }
                break;
            }
            case '.':
            {
                MT_ALSA_d_T_device_id device_id;
                std::cout << "Enter a Target : ";
                std::cin >> device_id.device_id;
                std::cout << std::endl;
                
                err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                    pptrHandle, MT_ALSA_d_SetTarget_NADAC, 
                    &device_id, sizeof(MT_ALSA_d_T_device_id), 
                    NULL, 0);
                    
                std::cout << "Set NADAC target ";
                switch (err_code)
                {
                    case MT_ALSA_d_E_SUCCESSFULL:
                        std::cout << "sucessfully completed." << std::endl;
                        break;
                    case MT_ALSA_d_E_NO_CONNECTION:
                        std::cout << "failed. The selected NADAC is not connected." << std::endl;
                        break;
                    case MT_ALSA_d_E_INVALID_ARGUMENT:
                        std::cout << "failed. Device Id is invalid." << std::endl;
                        break;
                    default:
                        std::cout << "failed. Unexpected error " << err_code << std::endl;
                }
                break;
            }
            case '/':
            {
                bool no_error = true;
                MT_ALSA_d_T_device_name device_name;
                MT_ALSA_d_T_device_id device_id;
                device_id.device_id = 0;
                out_size = sizeof(MT_ALSA_d_T_device_text);
                do
                {               
                    err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                        pptrHandle, MT_ALSA_d_Get_NADAC_Name, 
                        &device_id, sizeof(MT_ALSA_d_T_device_id), 
                        &device_name, &out_size);
                    
                    std::cout << "Get NADAC name for id ";
                    switch (err_code)
                    {
                        case MT_ALSA_d_E_SUCCESSFULL:
                            std::cout << device_id.device_id << " : " << device_name.name << std::endl;
                            break;
                        case MT_ALSA_d_E_INVALID_ARGUMENT:
                            std::cout << device_id.device_id << " failed. Device Id is invalid." << std::endl;
                            no_error = false;
                            break;
                        default:
                            std::cout << device_id.device_id  << " failed. Unexpected error " << err_code << std::endl;
                    }
                    device_id.device_id++;
                }
                while (no_error);
                break;
            }   
            case '+':
            {
                err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                    pptrHandle, MT_ALSA_d_Set_NADAC_Source_ToPlayer, 
                    NULL, 0, NULL, &out_size);
                    
                std::cout << "Set NADAC source to player ";
                switch (err_code)
                {
                    case MT_ALSA_d_E_SUCCESSFULL:
                        std::cout << "sucessfully completed." << std::endl;
                        break;
                    case MT_ALSA_d_E_NO_CONNECTION:
                        std::cout << "failed. NADAC not connected." << std::endl;
                        break;
                    default:
                        std::cout << "failed. Unexpected error " << err_code << std::endl;
                }
                break;
            }
            case '1':
            {
                err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                    pptrHandle, MT_ALSA_d_Display_NADAC_IconPlay, 
                    NULL, 0, NULL, &out_size);
                
                std::cout << "Display NADAC play logo ";
                switch (err_code)
                {
                    case MT_ALSA_d_E_SUCCESSFULL:
                        std::cout << "sucessfully completed." << std::endl;
                        break;
                    case MT_ALSA_d_E_NO_CONNECTION:
                        std::cout << "failed. NADAC not connected." << std::endl;
                        break;
                    default:
                        std::cout << "failed. Unexpected error " << err_code << std::endl;
                }
                break;
            }
            case '2':
            {
                err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                    pptrHandle, MT_ALSA_d_Display_NADAC_IconPause, 
                    NULL, 0, NULL, &out_size);
                std::cout << "Display NADAC pause logo ";
                switch (err_code)
                {
                    case MT_ALSA_d_E_SUCCESSFULL:
                        std::cout << "sucessfully completed." << std::endl;
                        break;
                    case MT_ALSA_d_E_NO_CONNECTION:
                        std::cout << "failed. NADAC not connected." << std::endl;
                        break;
                    default:
                        std::cout << "failed. Unexpected error " << err_code << std::endl;
                }
                break;
            }               
            case '3':
            {
                err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                    pptrHandle, MT_ALSA_d_Display_NADAC_IconNone, 
                    NULL, 0, NULL, &out_size);
                std::cout << "Remove NADAC play logo ";
                switch (err_code)
                {
                    case MT_ALSA_d_E_SUCCESSFULL:
                        std::cout << "sucessfully completed." << std::endl;
                        break;
                    case MT_ALSA_d_E_NO_CONNECTION:
                        std::cout << "failed. NADAC not connected." << std::endl;
                        break;
                    default:
                        std::cout << "failed. Unexpected error " << err_code << std::endl;
                }
            }
            case 'T':
            {
                std::string text;
                std::cout << "Enter a text (leave blank for default) : ";
                std::cin >> text;
                std::cout << std::endl;
                
                MT_ALSA_d_T_device_text device_text;
                strcpy(device_text.name, text.c_str());
                err_code = (MT_ALSA_d_error_code)MTAL_IPC_SendIOCTL(
                    pptrHandle, MT_ALSA_d_Display_NADAC_TextInfo, 
                    &device_text, sizeof(MT_ALSA_d_T_device_text), 
                    NULL, 0);
                    
                std::cout << "Display a custom text ";
                switch (err_code)
                {
                    case MT_ALSA_d_E_SUCCESSFULL:
                        std::cout << "sucessfully completed." << std::endl;
                        break;
                    case MT_ALSA_d_E_NO_CONNECTION:
                        std::cout << "failed. The selected NADAC is not connected." << std::endl;
                        break;
                    default:
                        std::cout << "failed. Unexpected error " << err_code << std::endl;
                }
                break;
            }
        }
    } 
    while (true);

    MTAL_IPC_destroy(pptrHandle);
    return NULL;
}

void exit_handler(int s)
{
    //putchar('Q');
    printf("...Caught signal %d\n",s);
    exiting = true;
    //pthread_join(thread, NULL);
    pthread_kill(thread, 0);
    printf("Bye bye\n");
    exit(1);
}

int main()
{
    pthread_create(&thread, NULL, &run, NULL);

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    pause();

    return 0;
}

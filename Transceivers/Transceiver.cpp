#include "Transceiver.h"

//#include "../MaximumLength/generator.h"


#define MAX_MODULES 32

void transceiver_callback(void * arg, RadioMsg * msg)
{
    Transceiver * self = (Transceiver *) arg;

    self->notify(*msg);
}

void Transceiver::debug(RadioMsg msg)
{
    signal();
}

Transceiver::Transceiver(TransceiverNotify notify_cb, void * obj, double fs, double fc, double fif, double bw):
    notify_cb(notify_cb),
    obj(obj),
    SignaledThread(128),
    fs(fs),
    fc(fc),
    fif(fif),
    bw(bw),
    order(8),
    cycles_per_bit(5),
    spectrum_size(1 << 10)
{
    //generate_ml_sequence(&prefix_len, &prefix);

    /* Initialize Objects */
    tx_memory = new Memory();
    rx_memory = new Memory();
    modules = new Module*[MAX_MODULES];

    int k;
    for (k = 0; k < MAX_MODULES; k++)
    {
        modules[k] = NULL;
    }

    #ifdef QT_ENABLE
    controller = new PlotController(_argc, _argv);
    #endif

}

void Transceiver::process(RadioMsg msg)
{
    RadioData * data = (RadioData *) &msg;
    int k;

    switch (msg.type)
    {
        case PROCESS_DATA:
            if (modules[data->get_id() + 1] != NULL)
            {
                modules[data->get_id() + 1]->dispatch(&msg);
            }
            break;

        case NOTIFY_USER_REQUEST_QUIT:
            LOG("Notifying that user is requesting quit...\n");
            notify_cb(obj, &msg);
            break;

        case CMD_START:
        case CMD_STOP:
        case CMD_RESET_ALL:
        case CMD_RESET_TRANSMITTER:
        case CMD_RESET_RECEIVER:
        case CMD_SET_TRANSMIT_CHANNEL:
        case CMD_SET_RECEIVE_CHANNEL:
        case NOTIFY_PLL_LOST_LOCK:
        case NOTIFY_PACKET_HEADER_DETECTED:
        case NOTIFY_RECEIVER_RESET_CONDITION_DETECTED:
        case NOTIFY_DATA_RECEIVED:

            Log("BROADCASTING: %s\n", RadioMsgString[msg.type]);
            k = 0;
            while (modules[k] != NULL)
            {
                LOG("[%2d]: %s->%s\n", k, RadioMsgString[msg.type], modules[k]->name());
                modules[k]->dispatch(&msg);
                k++;
            }
            break;
    }
}



void Transceiver::start(bool block)
{
    RadioMsg msg(CMD_START);
    this->notify(msg);
#ifdef QT_ENABLE
    SignaledThread::start(false);

    /* start the plot controller */
    int ret = controller->run();
#else 
    SignaledThread::start(block);
#endif
}

void Transceiver::stop()
{
    RadioMsg msg(CMD_STOP);
    this->notify(msg);
    SignaledThread::stop();
}

Transceiver::~Transceiver()
{
    int k;
    
    stop();

    for (k = 0; k < MAX_MODULES; k++)
    {
        if (modules[k] != NULL)
        {
            LOG("Deleting Module #%d...\n", k);
            delete modules[k];
        }
    }
    LOG("delete [] modules...\n");
    delete [] modules;

    LOG("delete tx_memory...\n");
    delete tx_memory;

    LOG("delete rx_memory...\n");
    delete rx_memory;

    #ifdef QT_ENABLE
    LOG("delete controller...\n");
    delete controller;
    #endif
}

#include "CommunicationFactory.h"
#include "SerialCommunication.h"
#include "cancommunication.h"
#include "udpcommunication.h"

ICommunication *CommunicationFactory::createCommunication(CommunicationType type) {
    switch (type) {
        case CommunicationType::Serial:
            return new SerialCommunication();
        case CommunicationType::Can:
            return new CanCommunication();
        case CommunicationType::Udp:
            return new UdpCommunication();
        default:
            return nullptr;
    }
}

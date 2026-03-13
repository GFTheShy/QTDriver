#ifndef COMMUNICATIONFACTORY_H
#define COMMUNICATIONFACTORY_H

#include "ICommunication.h"
#include "CommunicationParam.h"

class CommunicationFactory {
public:
    static ICommunication *createCommunication(CommunicationType type);
};

#endif // COMMUNICATIONFACTORY_H

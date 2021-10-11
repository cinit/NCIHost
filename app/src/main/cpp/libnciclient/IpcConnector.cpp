//
// Created by kinit on 2021-10-11.
//

#include "IpcConnector.h"

IpcConnector *volatile IpcConnector::sInstance = nullptr;

IpcConnector::IpcConnector() {

}

IpcConnector::~IpcConnector() {

}

void IpcConnector::init() {

}

/**
 *  no regard to concurrency here, just let it go...
 */
IpcConnector &IpcConnector::getInstance() {
    if (sInstance == nullptr) {
        // sInstance will NEVER be released
        sInstance = new IpcConnector();
    }
    return *sInstance;
}

IpcProxy *IpcConnector::getIpcProxy() {
    return nullptr;
}

void IpcConnector::sendConnectRequest() {

}

bool IpcConnector::isConnected() {
    return getIpcProxy() != nullptr;
}

void IpcConnector::registerIpcStatusListener(IpcConnector::IpcStatusListener *listener) {

}

bool IpcConnector::unregisterIpcStatusListener(IpcConnector::IpcStatusListener *listener) {
    return false;
}

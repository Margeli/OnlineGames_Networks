#include "Networks.h"
#include "DeliveryManager.h"

Delivery * DeliveryManager::writeSequenceNumber(OutputMemoryStream & packet)
{
	return nullptr;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream & packet)
{
	return false;
}

bool DeliveryManager::hasSequenceNumberPendingAck() const
{
	return false;
}

bool DeliveryManager::writeSequenceNumbersPendingAck()
{
	return false;
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream & paket)
{
}

void DeliveryManager::processTimedOutPackets()
{
}

void DeliveryManager::clear()
{
}

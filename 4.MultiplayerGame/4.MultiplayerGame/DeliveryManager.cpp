#include "Networks.h"
#include "DeliveryManager.h"

Delivery * DeliveryManager::writeSequenceNumber(OutputMemoryStream & packet, DeliveryDelegate& _delegate)
{
	packet << nextSequenceNumber;

	Delivery delivery;
	delivery.dispatchTime = Time.time;
	delivery.sequenceNumber = nextSequenceNumber++;
	delivery.delegate = &_delegate;

	pendingDeliveries.push_back(delivery);
	
	return &delivery;
}

bool DeliveryManager::processSequenceNumber(const InputMemoryStream & packet)
{
	uint32 receivedSequenceNumber = 0;
	packet >> receivedSequenceNumber;
	if (expectedSequenceNumber == receivedSequenceNumber) {		
		pendingAckDeliveries.push_back(receivedSequenceNumber); 
		++expectedSequenceNumber;
		return true;
	}

	return false;
}

bool DeliveryManager::hasSequenceNumberPendingAck() const
{
	return !pendingAckDeliveries.empty();
}

void DeliveryManager::writeSequenceNumbersPendingAck(OutputMemoryStream &packet)
{	
	for (auto &it : pendingAckDeliveries) {
		uint32 sequenceNum = it;
		packet << sequenceNum;
	}	
	pendingAckDeliveries.clear();
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream & packet)
{
	while (packet.RemainingByteCount() > 0) {
		uint32 seqNumber = 0;
		packet >> seqNumber;
		bool deliveryFound = false;
		for(auto it = pendingDeliveries.begin(); it!=pendingDeliveries.end(); it++)
		{
			if ((*it).sequenceNumber == seqNumber) {
				if ((*it).delegate) {//not null
					(*it).delegate->onDeliverySuccess(this);
				}
				(*it).CleanUp();
				pendingDeliveries.erase(it);
				deliveryFound = true;
				break;
			}			
		}
		if (!deliveryFound) {
		
			//error: sequence number to acknowledge is missing!
			int i = 101;
		}
	}
}

void DeliveryManager::processTimedOutPackets()
{
	std::vector<int> deliveryIndexToDelete;
	for (int i = 0; i < pendingDeliveries.size(); i++)
	{
		if (Time.time - pendingDeliveries[i].dispatchTime > PACKET_DELIVERY_TIMEOUT_SECONDS) {
			if (pendingDeliveries[i].delegate)//not null
			{
				pendingDeliveries[i].delegate->onDeliveryFailure(this);
				pendingDeliveries[i].CleanUp();
			}

			deliveryIndexToDelete.push_back(i);
		}
	}
	for (int i = deliveryIndexToDelete.size() - 1; i >= 0; i--) {
		pendingDeliveries.erase(pendingDeliveries.begin() + deliveryIndexToDelete[i]);
	}
	deliveryIndexToDelete.clear();
}

void DeliveryManager::clear()
{
	for (auto &it : pendingDeliveries) {
		it.CleanUp();
	}
	pendingDeliveries.clear();

	pendingAckDeliveries.clear();
	nextSequenceNumber = 0;
	expectedSequenceNumber = 0;
}

void DeliveryManager::restart()
{
	nextSequenceNumber = 0;
	expectedSequenceNumber = 0;
}



void Delivery::CleanUp()
{
	
	if (delegate) {
		delete delegate;
		delegate = nullptr;
	}
}

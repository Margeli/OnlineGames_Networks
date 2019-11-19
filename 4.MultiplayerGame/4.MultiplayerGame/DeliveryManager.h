#pragma once

class DeliveryManager;

class DeliveryDelegate
{
public:
	virtual void onDeliverySuccess(DeliveryManager* deliveryManager) = 0;
	virtual void onDeliveryFailure(DeliveryManager* deliveryManager) = 0;
};



struct Delivery
{
	uint32 sequenceNumber = 0;
	double dispatchTime = 0.0f;
	DeliveryDelegate* delegate = nullptr;

	void CleanUp();
};
	
class DeliveryManager
{
public:
	//For sender to write a new seq. numner into a packet
	Delivery* writeSequenceNumber(OutputMemoryStream &packet, DeliveryDelegate &delegate);

	//For receivers to process the seq. number from an incoming packet
	bool processSequenceNumber(const InputMemoryStream &packet);

	//For receivers to weite ack'ed sequence numbers into a packet
	bool hasSequenceNumberPendingAck() const;
	void writeSequenceNumbersPendingAck(OutputMemoryStream &packet);

	//For senders to process ack'ed seq. numbers from a packet
	void processAckdSequenceNumbers(const InputMemoryStream &paket);
	void processTimedOutPackets();

	void clear();

	void resendLostDelivery(Delivery* del);

private:
	
	// Private members (sender site)
	// - The next outgoing sequence number
	// - A list of pending deliveries

	uint32 nextSequenceNumber = 0;
	std::vector<Delivery> pendingDeliveries;

	// Private members (receiver site)
	// - The next expected sequence number
	// - A list of sequence numbers pending ack

	uint32 expectedSequenceNumber = 0;
	std::vector<uint32> pendingAckDeliveries;
};



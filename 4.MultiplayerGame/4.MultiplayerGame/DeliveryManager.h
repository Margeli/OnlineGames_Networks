#pragma once
class DeliveryManager
{
public:
	//For sender to write a new seq. numner into a packet
	Delivery* writeSequenceNumber(OutputMemoryStream &packet);

	//For receivers to process the seq. number from an incoming packet
	bool processSequenceNumber(const InputMemoryStream &packet);

	//For receivers to weite ack'ed sequence numbers into a packet
	bool hasSequenceNumberPendingAck() const;
	bool writeSequenceNumbersPendingAck();

	//For senders to process ack'ed seq. numbers from a packet
	void processAckdSequenceNumbers(const InputMemoryStream &paket);
	void processTimedOutPackets();

	void clear();

private:
	
	// Private members (sender site)
	// - The next outgoing sequence number
	// - A list of pending deliveries

	// Private members (receiver site)
	// - The next expected sequence number
	// - A list of sequence numbers pending ack
};

class DeliveryDelegate
{
public:
	virtual void onDeliverySuccess(DeliveryManager* deliveryManger) = 0;
	virtual void onDeliveryFailure(DeliveryManager* deliveryManger) = 0;
};

struct Delivery 
{
	uint32 sequenceNumber = 0;
	double dispathTime = 0.0f;
	DeliveryDelegate* delegate = nullptr;
};


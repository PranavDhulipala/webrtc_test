#ifndef WEBRTC_TESTER_H
#define WEBRTC_TESTER_H

#include <webrtc/api/peer_connection_interface.h>

// Dummy observer for SetSessionDescription. Used to handle success or failure callbacks.
class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
  static DummySetSessionDescriptionObserver* Create();
  virtual void OnSuccess() override;
  virtual void OnFailure(webrtc::RTCError error) override;
};

// Dummy observer for CreateSessionDescription. Used to handle success or failure callbacks.
class DummyCreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
public:
  static DummyCreateSessionDescriptionObserver* Create();
  virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  virtual void OnFailure(webrtc::RTCError error) override;
};

// Dummy observer for PeerConnection. Used to handle various events related to the connection.
class DummyPeerConnectionObserver : public webrtc::PeerConnectionObserver {
public:
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
  void OnRenegotiationNeeded() override;
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
};

#endif // WEBRTC_TESTER_H

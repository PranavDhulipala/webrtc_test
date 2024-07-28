#include <webrtc_tester/webrtc_tester.h>
#include <iostream>
#include <memory>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/create_peerconnection_factory.h>
#include <webrtc/rtc_base/thread.h>
#include <webrtc/modules/audio_device/include/audio_device.h>
#include <webrtc/api/audio_codecs/builtin_audio_decoder_factory.h>
#include <webrtc/api/audio_codecs/builtin_audio_encoder_factory.h>
#include <webrtc/api/audio_options.h>
#include <webrtc/api/task_queue/default_task_queue_factory.h>
#include <webrtc/p2p/client/basic_port_allocator.h>
#include <webrtc/rtc_base/ssl_adapter.h>
#include <webrtc/rtc_base/logging.h>

// Implementation of DummySetSessionDescriptionObserver

DummySetSessionDescriptionObserver* DummySetSessionDescriptionObserver::Create() {
  return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
}

void DummySetSessionDescriptionObserver::OnSuccess() {
  std::cout << "SetSessionDescription success." << std::endl;
}

void DummySetSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  std::cout << "SetSessionDescription failed: " << error.message() << std::endl;
}

// Implementation of DummyCreateSessionDescriptionObserver

DummyCreateSessionDescriptionObserver* DummyCreateSessionDescriptionObserver::Create() {
  return new rtc::RefCountedObject<DummyCreateSessionDescriptionObserver>();
}

void DummyCreateSessionDescriptionObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  std::cout << "CreateSessionDescription success." << std::endl;
}

void DummyCreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  std::cout << "CreateSessionDescription failed: " << error.message() << std::endl;
}

// Implementation of DummyPeerConnectionObserver

void DummyPeerConnectionObserver::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
  std::cout << "Signaling state changed: " << new_state << std::endl;
}

void DummyPeerConnectionObserver::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
  std::cout << "Stream added: " << stream->id() << std::endl;
}

void DummyPeerConnectionObserver::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
  std::cout << "Stream removed: " << stream->id() << std::endl;
}

void DummyPeerConnectionObserver::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
  std::cout << "Data channel created: " << data_channel->label() << std::endl;
}

void DummyPeerConnectionObserver::OnRenegotiationNeeded() {
  std::cout << "Renegotiation needed." << std::endl;
}

void DummyPeerConnectionObserver::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  std::cout << "ICE connection state changed: " << new_state << std::endl;
}

void DummyPeerConnectionObserver::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
  std::cout << "ICE gathering state changed: " << new_state << std::endl;
}

void DummyPeerConnectionObserver::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  std::cout << "ICE candidate: " << candidate->sdp_mline_index() << " " << candidate->candidate().ToString() << std::endl;
}

// Main function to test WebRTC functionalities
int main() {
  std::cout << "Testing WebRTC Package!" << std::endl;

  // Initialize logging for debugging purposes
  rtc::LogMessage::LogToDebug(rtc::LS_INFO); // Log to debug with level INFO
  rtc::LogMessage::LogTimestamps();          // Log timestamps
  rtc::LogMessage::LogThreads();             // Log thread IDs

  // Initialize SSL, which is required for WebRTC
  rtc::InitializeSSL();

  // Declare a scoped_refptr for PeerConnectionInterface to ensure it's properly managed
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;

  if (peer_connection == nullptr) {
    std::cout << "WebRTC PeerConnectionInterface initialized successfully." << std::endl;
  } else {
    std::cout << "Failed to initialize WebRTC PeerConnectionInterface." << std::endl;
  }

  // Create threads for networking, working, and signaling
  rtc::Thread* network_thread = rtc::Thread::Create().release();  // Create and release a network thread
  rtc::Thread* worker_thread = rtc::Thread::Create().release();   // Create and release a worker thread
  rtc::Thread* signaling_thread = rtc::Thread::Create().release();// Create and release a signaling thread
  network_thread->Start();    // Start the network thread
  worker_thread->Start();     // Start the worker thread
  signaling_thread->Start();  // Start the signaling thread

  // Create a TaskQueueFactory needed by the PeerConnectionFactory
  auto task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();

  // Create a dummy Audio Device Module (ADM)
  rtc::scoped_refptr<webrtc::AudioDeviceModule> adm = webrtc::AudioDeviceModule::Create(
      webrtc::AudioDeviceModule::kDummyAudio, task_queue_factory.get());

  // Create the PeerConnectionFactory
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory =
      webrtc::CreatePeerConnectionFactory(
          network_thread,             // Network thread
          worker_thread,              // Worker thread
          signaling_thread,           // Signaling thread
          adm,                        // Audio Device Module
          webrtc::CreateBuiltinAudioEncoderFactory(),  // Audio encoder factory
          webrtc::CreateBuiltinAudioDecoderFactory(),  // Audio decoder factory
          nullptr,  // Video encoder factory
          nullptr,  // Video decoder factory
          nullptr,  // Audio mixer
          nullptr,  // Audio processing
          nullptr,  // Audio frame processor
          nullptr   // Field trials view
      );

  if (peer_connection_factory == nullptr) {
    std::cerr << "Failed to create PeerConnectionFactory." << std::endl;
    return -1;
  } else {
    std::cout << "WebRTC PeerConnectionFactory created successfully." << std::endl;
  }

  // Configure the PeerConnection with an ICE server
  webrtc::PeerConnectionInterface::RTCConfiguration config; // Configuration for the PeerConnection
  webrtc::PeerConnectionInterface::IceServer ice_server;    // ICE server configuration
  ice_server.uri = "stun:stun.l.google.com:19302";          // URI for the STUN server
  config.servers.push_back(ice_server);                     // Add the ICE server to the configuration

  // Log ICE server configuration
  std::cout << "Configured ICE Server: " << ice_server.uri << std::endl;

  // Create PeerConnectionDependencies and pass the observer
  webrtc::PeerConnectionDependencies dependencies(new DummyPeerConnectionObserver());

  // Create the PeerConnection
  peer_connection = peer_connection_factory->CreatePeerConnection(config, std::move(dependencies));

  if (peer_connection == nullptr) {
    std::cerr << "Failed to create PeerConnection. Possible reasons could be:" << std::endl;
    std::cerr << "1. Incorrect RTCConfiguration." << std::endl;
    std::cerr << "2. Missing or invalid ICE server configurations." << std::endl;
    std::cerr << "3. Dependencies or constraints not met." << std::endl;
    return -1;
  } else {
    std::cout << "WebRTC PeerConnection created successfully." << std::endl;

    // Create a DataChannel
    webrtc::DataChannelInit data_channel_config;  // Configuration for the data channel
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel =
        peer_connection->CreateDataChannel("test_channel", &data_channel_config);

    if (data_channel == nullptr) {
      std::cerr << "Failed to create DataChannel." << std::endl;
    } else {
      std::cout << "WebRTC DataChannel created successfully." << std::endl;
    }

    // Create an offer for a WebRTC session
    peer_connection->CreateOffer(
        DummyCreateSessionDescriptionObserver::Create(),    // Observer for the offer creation
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());  // Options for the offer/answer

    std::cout << "CreateOffer called." << std::endl;
  }

  // Cleanup SSL when done
  rtc::CleanupSSL();

  return 0;
}

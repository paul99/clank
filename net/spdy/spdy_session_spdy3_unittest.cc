// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_session.h"

#include "base/location.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/pending_task.h"
#include "base/string_util.h"
#include "net/base/cert_test_util.h"
#include "net/base/host_cache.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_log_unittest.h"
#include "net/base/test_data_directory.h"
#include "net/base/test_data_stream.h"
#include "net/spdy/spdy_io_buffer.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/spdy/spdy_session_test_util.h"
#include "net/spdy/spdy_stream.h"
#include "net/spdy/spdy_test_util_spdy3.h"
#include "testing/platform_test.h"

using namespace net::test_spdy3;

namespace net {

namespace {

static const char kTestUrl[] = "http://www.example.org/";
static const char kTestHost[] = "www.example.org";
static const int kTestPort = 80;

static int g_delta_seconds = 0;
base::TimeTicks TheNearFuture() {
  return base::TimeTicks::Now() + base::TimeDelta::FromSeconds(g_delta_seconds);
}

class ClosingDelegate : public SpdyStream::Delegate {
 public:
  ClosingDelegate(SpdyStream* stream) : stream_(stream) {}

  // SpdyStream::Delegate implementation:
  virtual bool OnSendHeadersComplete(int status) OVERRIDE {
    return true;
  }
  virtual int OnSendBody() OVERRIDE {
    return OK;
  }
  virtual int OnSendBodyComplete(int status, bool* eof) OVERRIDE {
    return OK;
  }
  virtual int OnResponseReceived(const SpdyHeaderBlock& response,
                                 base::Time response_time,
                                 int status) OVERRIDE {
    return OK;
  }
  virtual void OnHeadersSent() OVERRIDE {}
  virtual int OnDataReceived(const char* data, int length) OVERRIDE {
    return OK;
  }
  virtual void OnDataSent(int length) OVERRIDE {}
  virtual void OnClose(int status) OVERRIDE {
    stream_->Close();
  }
 private:
  SpdyStream* stream_;
};

class TestSpdyStreamDelegate : public SpdyStream::Delegate {
 public:
  explicit TestSpdyStreamDelegate(const CompletionCallback& callback)
      : callback_(callback) {}
  virtual ~TestSpdyStreamDelegate() {}

  virtual bool OnSendHeadersComplete(int status) OVERRIDE { return true; }

  virtual int OnSendBody() OVERRIDE {
    return ERR_UNEXPECTED;
  }

  virtual int OnSendBodyComplete(int /*status*/, bool* /*eof*/) OVERRIDE {
    return ERR_UNEXPECTED;
  }

  virtual int OnResponseReceived(const SpdyHeaderBlock& response,
                                 base::Time response_time,
                                 int status) OVERRIDE {
    return status;
  }

  virtual void OnHeadersSent() OVERRIDE {}
  virtual int OnDataReceived(const char* buffer, int bytes) OVERRIDE {
    return OK;
  }

  virtual void OnDataSent(int length) OVERRIDE {}

  virtual void OnClose(int status) OVERRIDE {
    CompletionCallback callback = callback_;
    callback_.Reset();
    callback.Run(OK);
  }

 private:
  CompletionCallback callback_;
};

} // namespace

class SpdySessionSpdy3Test : public PlatformTest {
 protected:
  SpdySessionSpdy3Test()
      : spdy_session_pool_(NULL),
        test_url_(kTestUrl),
        test_host_port_pair_(kTestHost, kTestPort),
        pair_(test_host_port_pair_, ProxyServer::Direct()) {
  }

  virtual void SetUp() {
    g_delta_seconds = 0;
  }

  void CreateDeterministicNetworkSession() {
    http_session_ =
        SpdySessionDependencies::SpdyCreateSessionDeterministic(&session_deps_);
    spdy_session_pool_ = http_session_->spdy_session_pool();
  }

  void CreateNetworkSession() {
    http_session_ =
        SpdySessionDependencies::SpdyCreateSession(&session_deps_);
    spdy_session_pool_ = http_session_->spdy_session_pool();
  }

  scoped_refptr<SpdySession> GetSession(const HostPortProxyPair& pair) {
    EXPECT_FALSE(spdy_session_pool_->HasSession(pair));
    scoped_refptr<SpdySession> session =
        spdy_session_pool_->Get(pair, BoundNetLog());
    EXPECT_TRUE(spdy_session_pool_->HasSession(pair));
    return session;
  }

  // Creates an initialized session to |pair_|.
  scoped_refptr<SpdySession> CreateInitializedSession() {
    scoped_refptr<SpdySession> session = GetSession(pair_);
    InitializeSession(http_session_.get(), session.get(), test_host_port_pair_);
    return session;
  }

  void InitializeSession(HttpNetworkSession* http_session,
                         SpdySession* session,
                         const HostPortPair& host_port_pair) {
    transport_params_ = new TransportSocketParams(
        host_port_pair, MEDIUM, false, false, OnHostResolutionCallback());

    scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
    EXPECT_EQ(OK, connection->Init(host_port_pair.ToString(),
                                   transport_params_, MEDIUM,
                                   CompletionCallback(),
                                   http_session->GetTransportSocketPool(
                                       HttpNetworkSession::NORMAL_SOCKET_POOL),
                                   BoundNetLog()));
    EXPECT_EQ(OK,
              session->InitializeWithSocket(connection.release(), false, OK));
  }

  scoped_refptr<TransportSocketParams> transport_params_;
  SpdySessionDependencies session_deps_;
  scoped_refptr<HttpNetworkSession> http_session_;
  SpdySessionPool* spdy_session_pool_;
  GURL test_url_;
  HostPortPair test_host_port_pair_;
  HostPortProxyPair pair_;
};

// Test the SpdyIOBuffer class.
TEST_F(SpdySessionSpdy3Test, SpdyIOBuffer) {
  std::priority_queue<SpdyIOBuffer> queue_;
  const size_t kQueueSize = 100;

  // Insert items with random priority and increasing buffer size.
  for (size_t index = 0; index < kQueueSize; ++index) {
    queue_.push(SpdyIOBuffer(
        new IOBufferWithSize(index + 1),
        index + 1,
        static_cast<RequestPriority>(rand() % NUM_PRIORITIES),
        NULL));
  }

  EXPECT_EQ(kQueueSize, queue_.size());

  // Verify items come out with decreasing priority or FIFO order.
  RequestPriority last_priority = NUM_PRIORITIES;
  size_t last_size = 0;
  for (size_t index = 0; index < kQueueSize; ++index) {
    SpdyIOBuffer buffer = queue_.top();
    EXPECT_LE(buffer.priority(), last_priority);
    if (buffer.priority() < last_priority)
      last_size = 0;
    EXPECT_LT(last_size, buffer.size());
    last_priority = buffer.priority();
    last_size = buffer.size();
    queue_.pop();
  }

  EXPECT_EQ(0u, queue_.size());
}

TEST_F(SpdySessionSpdy3Test, GoAway) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> goaway(ConstructSpdyGoAway());
  MockRead reads[] = {
    CreateMockRead(*goaway),
    MockRead(SYNCHRONOUS, 0, 0)  // EOF
  };
  StaticSocketDataProvider data(reads, arraysize(reads), NULL, 0);
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  ssl.SetNextProto(kProtoSPDY3);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  EXPECT_EQ(3, session->GetProtocolVersion());

  // Flush the SpdySession::OnReadComplete() task.
  MessageLoop::current()->RunUntilIdle();

  EXPECT_FALSE(spdy_session_pool_->HasSession(pair_));

  scoped_refptr<SpdySession> session2 = GetSession(pair_);

  // Delete the first session.
  session = NULL;

  // Delete the second session.
  spdy_session_pool_->Remove(session2);
  session2 = NULL;
}

TEST_F(SpdySessionSpdy3Test, ClientPing) {
  session_deps_.enable_ping = true;
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> read_ping(ConstructSpdyPing(1));
  MockRead reads[] = {
    CreateMockRead(*read_ping),
    MockRead(SYNCHRONOUS, 0, 0)  // EOF
  };
  scoped_ptr<SpdyFrame> write_ping(ConstructSpdyPing(1));
  MockWrite writes[] = {
    CreateMockWrite(*write_ping),
  };
  StaticSocketDataProvider data(
      reads, arraysize(reads), writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  EXPECT_EQ(OK, session->CreateStream(test_url_, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));

  scoped_ptr<TestSpdyStreamDelegate> delegate(
      new TestSpdyStreamDelegate(callback1.callback()));
  spdy_stream1->SetDelegate(delegate.get());

  base::TimeTicks before_ping_time = base::TimeTicks::Now();

  session->set_connection_at_risk_of_loss_time(base::TimeDelta::FromSeconds(0));
  session->set_hung_interval(base::TimeDelta::FromMilliseconds(50));

  session->SendPrefacePingIfNoneInFlight();

  EXPECT_EQ(OK, callback1.WaitForResult());

  session->CheckPingStatus(before_ping_time);

  EXPECT_EQ(0, session->pings_in_flight());
  EXPECT_GE(session->next_ping_id(), static_cast<uint32>(1));
  EXPECT_FALSE(session->check_ping_status_pending());
  EXPECT_GE(session->last_activity_time(), before_ping_time);

  EXPECT_FALSE(spdy_session_pool_->HasSession(pair_));

  // Delete the first session.
  session = NULL;
}

TEST_F(SpdySessionSpdy3Test, ServerPing) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> read_ping(ConstructSpdyPing(2));
  MockRead reads[] = {
    CreateMockRead(*read_ping),
    MockRead(SYNCHRONOUS, 0, 0)  // EOF
  };
  scoped_ptr<SpdyFrame> write_ping(ConstructSpdyPing(2));
  MockWrite writes[] = {
    CreateMockWrite(*write_ping),
  };
  StaticSocketDataProvider data(
      reads, arraysize(reads), writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  EXPECT_EQ(OK, session->CreateStream(test_url_, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  scoped_ptr<TestSpdyStreamDelegate> delegate(
      new TestSpdyStreamDelegate(callback1.callback()));
  spdy_stream1->SetDelegate(delegate.get());

  // Flush the SpdySession::OnReadComplete() task.
  MessageLoop::current()->RunUntilIdle();

  EXPECT_FALSE(spdy_session_pool_->HasSession(pair_));

  // Delete the session.
  session = NULL;
}

TEST_F(SpdySessionSpdy3Test, DeleteExpiredPushStreams) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);
  session_deps_.time_func = TheNearFuture;

  CreateNetworkSession();

  scoped_refptr<SpdySession> session = GetSession(pair_);

  // Give the session a SPDY3 framer.
  session->buffered_spdy_framer_.reset(new BufferedSpdyFramer(3, false));

  // Create the associated stream and add to active streams.
  scoped_ptr<SpdyHeaderBlock> request_headers(new SpdyHeaderBlock);
  (*request_headers)[":scheme"] = "http";
  (*request_headers)[":host"] = "www.google.com";
  (*request_headers)[":path"] = "/";

  scoped_refptr<SpdyStream> stream(
      new SpdyStream(session, false, session->net_log_));
  stream->set_spdy_headers(request_headers.Pass());
  session->ActivateStream(stream);

  SpdyHeaderBlock headers;
  headers[":scheme"] = "http";
  headers[":host"] = "www.google.com";
  headers[":path"] = "/a.dat";
  session->OnSynStream(2, 1, 0, 0, true, false, headers);

  // Verify that there is one unclaimed push stream.
  EXPECT_EQ(1u, session->num_unclaimed_pushed_streams());
  SpdySession::PushedStreamMap::iterator  iter  =
      session->unclaimed_pushed_streams_.find("http://www.google.com/a.dat");
  EXPECT_TRUE(session->unclaimed_pushed_streams_.end() != iter);

  // Shift time.
  g_delta_seconds = 301;

  headers[":scheme"] = "http";
  headers[":host"] = "www.google.com";
  headers[":path"] = "/b.dat";
  session->OnSynStream(4, 1, 0, 0, true, false, headers);

  // Verify that the second pushed stream evicted the first pushed stream.
  EXPECT_EQ(1u, session->num_unclaimed_pushed_streams());
  iter = session->unclaimed_pushed_streams_.find("http://www.google.com/b.dat");
  EXPECT_TRUE(session->unclaimed_pushed_streams_.end() != iter);

  // Delete the session.
  session = NULL;
}

TEST_F(SpdySessionSpdy3Test, FailedPing) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> read_ping(ConstructSpdyPing(1));
  MockRead reads[] = {
    CreateMockRead(*read_ping),
    MockRead(SYNCHRONOUS, 0, 0)  // EOF
  };
  scoped_ptr<SpdyFrame> write_ping(ConstructSpdyPing(1));
  MockWrite writes[] = {
    CreateMockWrite(*write_ping),
  };
  StaticSocketDataProvider data(
      reads, arraysize(reads), writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  EXPECT_EQ(OK, session->CreateStream(test_url_, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  scoped_ptr<TestSpdyStreamDelegate> delegate(
      new TestSpdyStreamDelegate(callback1.callback()));
  spdy_stream1->SetDelegate(delegate.get());

  session->set_connection_at_risk_of_loss_time(base::TimeDelta::FromSeconds(0));
  session->set_hung_interval(base::TimeDelta::FromSeconds(0));

  // Send a PING frame.
  session->WritePingFrame(1);
  EXPECT_LT(0, session->pings_in_flight());
  EXPECT_GE(session->next_ping_id(), static_cast<uint32>(1));
  EXPECT_TRUE(session->check_ping_status_pending());

  // Assert session is not closed.
  EXPECT_FALSE(session->IsClosed());
  EXPECT_LT(0u, session->num_active_streams() + session->num_created_streams());
  EXPECT_TRUE(spdy_session_pool_->HasSession(pair_));

  // We set last time we have received any data in 1 sec less than now.
  // CheckPingStatus will trigger timeout because hung interval is zero.
  base::TimeTicks now = base::TimeTicks::Now();
  session->last_activity_time_ = now - base::TimeDelta::FromSeconds(1);
  session->CheckPingStatus(now);

  EXPECT_TRUE(session->IsClosed());
  EXPECT_EQ(0u, session->num_active_streams());
  EXPECT_EQ(0u, session->num_unclaimed_pushed_streams());
  EXPECT_FALSE(spdy_session_pool_->HasSession(pair_));

  // Delete the first session.
  session = NULL;
}

class StreamReleaserCallback : public TestCompletionCallbackBase {
 public:
  StreamReleaserCallback(SpdySession* session,
                         SpdyStream* first_stream)
      : session_(session),
        first_stream_(first_stream),
        ALLOW_THIS_IN_INITIALIZER_LIST(callback_(
            base::Bind(&StreamReleaserCallback::OnComplete,
                       base::Unretained(this)))) {
  }

  virtual ~StreamReleaserCallback() {}

  scoped_refptr<SpdyStream>* stream() { return &stream_; }

  const CompletionCallback& callback() const { return callback_; }

 private:
  void OnComplete(int result) {
    session_->CloseSessionOnError(ERR_FAILED, false, "On complete.");
    session_ = NULL;
    first_stream_->Cancel();
    first_stream_ = NULL;
    stream_->Cancel();
    stream_ = NULL;
    SetResult(result);
  }

  scoped_refptr<SpdySession> session_;
  scoped_refptr<SpdyStream> first_stream_;
  scoped_refptr<SpdyStream> stream_;
  CompletionCallback callback_;
};

// TODO(kristianm): Could also test with more sessions where some are idle,
// and more than one session to a HostPortPair.
TEST_F(SpdySessionSpdy3Test, CloseIdleSessions) {
  CreateNetworkSession();

  // Set up session 1
  const std::string kTestHost1("http://www.a.com");
  HostPortPair test_host_port_pair1(kTestHost1, 80);
  HostPortProxyPair pair1(test_host_port_pair1, ProxyServer::Direct());
  scoped_refptr<SpdySession> session1 = GetSession(pair1);
  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1(kTestHost1);
  EXPECT_EQ(OK, session1->CreateStream(url1, MEDIUM, &spdy_stream1,
                                       BoundNetLog(), callback1.callback()));

  // Set up session 2
  const std::string kTestHost2("http://www.b.com");
  HostPortPair test_host_port_pair2(kTestHost2, 80);
  HostPortProxyPair pair2(test_host_port_pair2, ProxyServer::Direct());
  scoped_refptr<SpdySession> session2 = GetSession(pair2);
  scoped_refptr<SpdyStream> spdy_stream2;
  TestCompletionCallback callback2;
  GURL url2(kTestHost2);
  EXPECT_EQ(OK, session2->CreateStream(url2, MEDIUM, &spdy_stream2,
                                       BoundNetLog(), callback2.callback()));

  // Set up session 3
  const std::string kTestHost3("http://www.c.com");
  HostPortPair test_host_port_pair3(kTestHost3, 80);
  HostPortProxyPair pair3(test_host_port_pair3, ProxyServer::Direct());
  scoped_refptr<SpdySession> session3 = GetSession(pair3);
  scoped_refptr<SpdyStream> spdy_stream3;
  TestCompletionCallback callback3;
  GURL url3(kTestHost3);
  EXPECT_EQ(OK, session3->CreateStream(url3, MEDIUM, &spdy_stream3,
                                       BoundNetLog(), callback3.callback()));

  // All sessions are active and not closed
  EXPECT_TRUE(session1->is_active());
  EXPECT_FALSE(session1->IsClosed());
  EXPECT_TRUE(session2->is_active());
  EXPECT_FALSE(session2->IsClosed());
  EXPECT_TRUE(session3->is_active());
  EXPECT_FALSE(session3->IsClosed());

  // Should not do anything, all are active
  spdy_session_pool_->CloseIdleSessions();
  EXPECT_TRUE(session1->is_active());
  EXPECT_FALSE(session1->IsClosed());
  EXPECT_TRUE(session2->is_active());
  EXPECT_FALSE(session2->IsClosed());
  EXPECT_TRUE(session3->is_active());
  EXPECT_FALSE(session3->IsClosed());

  // Make sessions 1 and 3 inactive, but keep them open.
  // Session 2 still open and active
  session1->CloseCreatedStream(spdy_stream1, OK);
  session3->CloseCreatedStream(spdy_stream3, OK);
  EXPECT_FALSE(session1->is_active());
  EXPECT_FALSE(session1->IsClosed());
  EXPECT_TRUE(session2->is_active());
  EXPECT_FALSE(session2->IsClosed());
  EXPECT_FALSE(session3->is_active());
  EXPECT_FALSE(session3->IsClosed());

  // Should close session 1 and 3, 2 should be left open
  spdy_session_pool_->CloseIdleSessions();
  EXPECT_FALSE(session1->is_active());
  EXPECT_TRUE(session1->IsClosed());
  EXPECT_TRUE(session2->is_active());
  EXPECT_FALSE(session2->IsClosed());
  EXPECT_FALSE(session3->is_active());
  EXPECT_TRUE(session3->IsClosed());

  // Should not do anything
  spdy_session_pool_->CloseIdleSessions();
  EXPECT_TRUE(session2->is_active());
  EXPECT_FALSE(session2->IsClosed());

  // Make 2 not active
  session2->CloseCreatedStream(spdy_stream2, OK);
  EXPECT_FALSE(session2->is_active());
  EXPECT_FALSE(session2->IsClosed());

  // This should close session 2
  spdy_session_pool_->CloseIdleSessions();
  EXPECT_FALSE(session2->is_active());
  EXPECT_TRUE(session2->IsClosed());
}

// Start with max concurrent streams set to 1.  Request two streams.  Receive a
// settings frame setting max concurrent streams to 2.  Have the callback
// release the stream, which releases its reference (the last) to the session.
// Make sure nothing blows up.
// http://crbug.com/57331
TEST_F(SpdySessionSpdy3Test, OnSettings) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  SettingsMap new_settings;
  const SpdySettingsIds kSpdySettingsIds1 = SETTINGS_MAX_CONCURRENT_STREAMS;
  const uint32 max_concurrent_streams = 2;
  new_settings[kSpdySettingsIds1] =
      SettingsFlagsAndValue(SETTINGS_FLAG_NONE, max_concurrent_streams);

  // Set up the socket so we read a SETTINGS frame that raises max concurrent
  // streams to 2.
  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> settings_frame(ConstructSpdySettings(new_settings));
  MockRead reads[] = {
    CreateMockRead(*settings_frame),
    MockRead(SYNCHRONOUS, 0, 0)  // EOF
  };

  StaticSocketDataProvider data(reads, arraysize(reads), NULL, 0);
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  // Initialize the SpdySetting with 1 max concurrent streams.
  spdy_session_pool_->http_server_properties()->SetSpdySetting(
      test_host_port_pair_,
      kSpdySettingsIds1,
      SETTINGS_FLAG_PLEASE_PERSIST,
      1);

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  // Create 2 streams.  First will succeed.  Second will be pending.
  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  EXPECT_EQ(OK,
            session->CreateStream(test_url_, MEDIUM, &spdy_stream1,
                                  BoundNetLog(), callback1.callback()));

  StreamReleaserCallback stream_releaser(session, spdy_stream1);

  ASSERT_EQ(ERR_IO_PENDING,
            session->CreateStream(test_url_, MEDIUM, stream_releaser.stream(),
                                  BoundNetLog(), stream_releaser.callback()));

  // Make sure |stream_releaser| holds the last refs.
  session = NULL;
  spdy_stream1 = NULL;

  EXPECT_EQ(OK, stream_releaser.WaitForResult());
}

// Start with max concurrent streams set to 1.  Request two streams.  When the
// first completes, have the callback close itself, which should trigger the
// second stream creation.  Then cancel that one immediately.  Don't crash.
// http://crbug.com/63532
TEST_F(SpdySessionSpdy3Test, CancelPendingCreateStream) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockRead reads[] = {
    MockRead(SYNCHRONOUS, ERR_IO_PENDING)  // Stall forever.
  };

  StaticSocketDataProvider data(reads, arraysize(reads), NULL, 0);
  MockConnect connect_data(SYNCHRONOUS, OK);

  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  // Initialize the SpdySetting with 1 max concurrent streams.
  spdy_session_pool_->http_server_properties()->SetSpdySetting(
      test_host_port_pair_,
      SETTINGS_MAX_CONCURRENT_STREAMS,
      SETTINGS_FLAG_PLEASE_PERSIST,
      1);

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  // Use scoped_ptr to let us invalidate the memory when we want to, to trigger
  // a valgrind error if the callback is invoked when it's not supposed to be.
  scoped_ptr<TestCompletionCallback> callback(new TestCompletionCallback);

  // Create 2 streams.  First will succeed.  Second will be pending.
  scoped_refptr<SpdyStream> spdy_stream1;
  ASSERT_EQ(OK, session->CreateStream(test_url_, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback->callback()));

  scoped_refptr<SpdyStream> spdy_stream2;
  ASSERT_EQ(ERR_IO_PENDING,
            session->CreateStream(test_url_, MEDIUM, &spdy_stream2,
                                  BoundNetLog(), callback->callback()));

  // Release the first one, this will allow the second to be created.
  spdy_stream1->Cancel();
  spdy_stream1 = NULL;

  session->CancelPendingCreateStreams(&spdy_stream2);
  callback.reset();

  // Should not crash when running the pending callback.
  MessageLoop::current()->RunUntilIdle();
}

TEST_F(SpdySessionSpdy3Test, SendInitialSettingsOnNewSession) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockRead reads[] = {
    MockRead(SYNCHRONOUS, ERR_IO_PENDING)  // Stall forever.
  };

  SettingsMap settings;
  const SpdySettingsIds kSpdySettingsIds1 = SETTINGS_MAX_CONCURRENT_STREAMS;
  const SpdySettingsIds kSpdySettingsIds2 = SETTINGS_INITIAL_WINDOW_SIZE;
  const uint32 kInitialRecvWindowSize = 10 * 1024 * 1024;
  settings[kSpdySettingsIds1] =
      SettingsFlagsAndValue(SETTINGS_FLAG_NONE, kMaxConcurrentPushedStreams);
  settings[kSpdySettingsIds2] =
      SettingsFlagsAndValue(SETTINGS_FLAG_NONE, kInitialRecvWindowSize);
  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> settings_frame(ConstructSpdySettings(settings));
  MockWrite writes[] = {
    CreateMockWrite(*settings_frame),
  };
  session_deps_.initial_recv_window_size = kInitialRecvWindowSize;

  StaticSocketDataProvider data(
      reads, arraysize(reads), writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  SpdySessionPoolPeer pool_peer(spdy_session_pool_);
  pool_peer.EnableSendingInitialSettings(true);

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  MessageLoop::current()->RunUntilIdle();
  EXPECT_TRUE(data.at_write_eof());
}

TEST_F(SpdySessionSpdy3Test, SendSettingsOnNewSession) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockRead reads[] = {
    MockRead(SYNCHRONOUS, ERR_IO_PENDING)  // Stall forever.
  };

  // Create the bogus setting that we want to verify is sent out.
  // Note that it will be marked as SETTINGS_FLAG_PERSISTED when sent out. But
  // to persist it into the HttpServerProperties, we need to mark as
  // SETTINGS_FLAG_PLEASE_PERSIST.
  SettingsMap settings;
  const SpdySettingsIds kSpdySettingsIds1 = SETTINGS_UPLOAD_BANDWIDTH;
  const uint32 kBogusSettingValue = 0xCDCD;
  settings[kSpdySettingsIds1] =
      SettingsFlagsAndValue(SETTINGS_FLAG_PERSISTED, kBogusSettingValue);
  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> settings_frame(ConstructSpdySettings(settings));
  MockWrite writes[] = {
    CreateMockWrite(*settings_frame),
  };

  StaticSocketDataProvider data(
      reads, arraysize(reads), writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  spdy_session_pool_->http_server_properties()->SetSpdySetting(
      test_host_port_pair_,
      kSpdySettingsIds1,
      SETTINGS_FLAG_PLEASE_PERSIST,
      kBogusSettingValue);

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  MessageLoop::current()->RunUntilIdle();
  EXPECT_TRUE(data.at_write_eof());
}

namespace {
// This test has two variants, one for each style of closing the connection.
// If |clean_via_close_current_sessions| is false, the sessions are closed
// manually, calling SpdySessionPool::Remove() directly.  If it is true,
// sessions are closed with SpdySessionPool::CloseCurrentSessions().
void IPPoolingTest(bool clean_via_close_current_sessions) {
  const int kTestPort = 80;
  struct TestHosts {
    std::string name;
    std::string iplist;
    HostPortProxyPair pair;
    AddressList addresses;
  } test_hosts[] = {
    { "www.foo.com",    "192.0.2.33,192.168.0.1,192.168.0.5" },
    { "images.foo.com", "192.168.0.2,192.168.0.3,192.168.0.5,192.0.2.33" },
    { "js.foo.com",     "192.168.0.4,192.168.0.3" },
  };

  SpdySessionDependencies session_deps;
  session_deps.host_resolver->set_synchronous_mode(true);
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(test_hosts); i++) {
    session_deps.host_resolver->rules()->AddIPLiteralRule(test_hosts[i].name,
        test_hosts[i].iplist, "");

    // This test requires that the HostResolver cache be populated.  Normal
    // code would have done this already, but we do it manually.
    HostResolver::RequestInfo info(HostPortPair(test_hosts[i].name, kTestPort));
    session_deps.host_resolver->Resolve(
        info, &test_hosts[i].addresses, CompletionCallback(), NULL,
        BoundNetLog());

    // Setup a HostPortProxyPair
    test_hosts[i].pair = HostPortProxyPair(
        HostPortPair(test_hosts[i].name, kTestPort), ProxyServer::Direct());
  }

  MockConnect connect_data(SYNCHRONOUS, OK);
  MockRead reads[] = {
    MockRead(SYNCHRONOUS, ERR_IO_PENDING)  // Stall forever.
  };

  StaticSocketDataProvider data(reads, arraysize(reads), NULL, 0);
  data.set_connect_data(connect_data);
  session_deps.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps.socket_factory->AddSSLSocketDataProvider(&ssl);

  scoped_refptr<HttpNetworkSession> http_session(
      SpdySessionDependencies::SpdyCreateSession(&session_deps));

  // Setup the first session to the first host.
  SpdySessionPool* spdy_session_pool(http_session->spdy_session_pool());
  EXPECT_FALSE(spdy_session_pool->HasSession(test_hosts[0].pair));
  scoped_refptr<SpdySession> session =
      spdy_session_pool->Get(test_hosts[0].pair, BoundNetLog());
  EXPECT_TRUE(spdy_session_pool->HasSession(test_hosts[0].pair));

  HostPortPair test_host_port_pair(test_hosts[0].name, kTestPort);

  scoped_refptr<TransportSocketParams> transport_params(
      new TransportSocketParams(test_host_port_pair,
                                MEDIUM,
                                false,
                                false,
                                OnHostResolutionCallback()));
  scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
  EXPECT_EQ(OK, connection->Init(test_host_port_pair.ToString(),
                                 transport_params, MEDIUM, CompletionCallback(),
                                 http_session->GetTransportSocketPool(
                                     HttpNetworkSession::NORMAL_SOCKET_POOL),
                                 BoundNetLog()));
  EXPECT_EQ(OK, session->InitializeWithSocket(connection.release(), false, OK));

  // TODO(rtenneti): MockClientSocket::GetPeerAddress return's 0 as the port
  // number. Fix it to return port 80 and then use GetPeerAddress to AddAlias.
  SpdySessionPoolPeer pool_peer(spdy_session_pool);
  pool_peer.AddAlias(test_hosts[0].addresses.front(), test_hosts[0].pair);

  // Flush the SpdySession::OnReadComplete() task.
  MessageLoop::current()->RunUntilIdle();

  // The third host has no overlap with the first, so it can't pool IPs.
  EXPECT_FALSE(spdy_session_pool->HasSession(test_hosts[2].pair));

  // The second host overlaps with the first, and should IP pool.
  EXPECT_TRUE(spdy_session_pool->HasSession(test_hosts[1].pair));

  // Verify that the second host, through a proxy, won't share the IP.
  HostPortProxyPair proxy_pair(test_hosts[1].pair.first,
      ProxyServer::FromPacString("HTTP http://proxy.foo.com/"));
  EXPECT_FALSE(spdy_session_pool->HasSession(proxy_pair));

  // Overlap between 2 and 3 does is not transitive to 1.
  EXPECT_FALSE(spdy_session_pool->HasSession(test_hosts[2].pair));

  // Create a new session to host 2.
  scoped_refptr<SpdySession> session2 =
      spdy_session_pool->Get(test_hosts[2].pair, BoundNetLog());

  // Verify that we have sessions for everything.
  EXPECT_TRUE(spdy_session_pool->HasSession(test_hosts[0].pair));
  EXPECT_TRUE(spdy_session_pool->HasSession(test_hosts[1].pair));
  EXPECT_TRUE(spdy_session_pool->HasSession(test_hosts[2].pair));

  // Grab the session to host 1 and verify that it is the same session
  // we got with host 0, and that is a different than host 2's session.
  scoped_refptr<SpdySession> session1 =
      spdy_session_pool->Get(test_hosts[1].pair, BoundNetLog());
  EXPECT_EQ(session.get(), session1.get());
  EXPECT_NE(session2.get(), session1.get());

  // Remove the aliases and observe that we still have a session for host1.
  pool_peer.RemoveAliases(test_hosts[0].pair);
  pool_peer.RemoveAliases(test_hosts[1].pair);
  EXPECT_TRUE(spdy_session_pool->HasSession(test_hosts[1].pair));

  // Expire the host cache
  session_deps.host_resolver->GetHostCache()->clear();
  EXPECT_TRUE(spdy_session_pool->HasSession(test_hosts[1].pair));

  // Cleanup the sessions.
  if (!clean_via_close_current_sessions) {
    spdy_session_pool->Remove(session);
    session = NULL;
    spdy_session_pool->Remove(session2);
    session2 = NULL;
  } else {
    spdy_session_pool->CloseCurrentSessions(net::ERR_ABORTED);
  }

  // Verify that the map is all cleaned up.
  EXPECT_FALSE(spdy_session_pool->HasSession(test_hosts[0].pair));
  EXPECT_FALSE(spdy_session_pool->HasSession(test_hosts[1].pair));
  EXPECT_FALSE(spdy_session_pool->HasSession(test_hosts[2].pair));
}

}  // namespace

TEST_F(SpdySessionSpdy3Test, IPPooling) {
  IPPoolingTest(false);
}

TEST_F(SpdySessionSpdy3Test, IPPoolingCloseCurrentSessions) {
  IPPoolingTest(true);
}

TEST_F(SpdySessionSpdy3Test, ClearSettingsStorageOnIPAddressChanged) {
  CreateNetworkSession();

  HttpServerProperties* test_http_server_properties =
      spdy_session_pool_->http_server_properties();
  SettingsFlagsAndValue flags_and_value1(SETTINGS_FLAG_PLEASE_PERSIST, 2);
  test_http_server_properties->SetSpdySetting(
      test_host_port_pair_,
      SETTINGS_MAX_CONCURRENT_STREAMS,
      SETTINGS_FLAG_PLEASE_PERSIST,
      2);
  EXPECT_NE(0u, test_http_server_properties->GetSpdySettings(
      test_host_port_pair_).size());
  spdy_session_pool_->OnIPAddressChanged();
  EXPECT_EQ(0u, test_http_server_properties->GetSpdySettings(
      test_host_port_pair_).size());
}

TEST_F(SpdySessionSpdy3Test, CloseSessionOnError) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> goaway(ConstructSpdyGoAway());
  MockRead reads[] = {
    CreateMockRead(*goaway),
    MockRead(SYNCHRONOUS, 0, 0)  // EOF
  };

  StaticSocketDataProvider data(reads, arraysize(reads), NULL, 0);
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  CapturingBoundNetLog log;
  scoped_refptr<SpdySession> session =
      spdy_session_pool_->Get(pair_, log.bound());
  EXPECT_TRUE(spdy_session_pool_->HasSession(pair_));

  InitializeSession(http_session_.get(), session.get(), test_host_port_pair_);

  // Flush the SpdySession::OnReadComplete() task.
  MessageLoop::current()->RunUntilIdle();

  EXPECT_FALSE(spdy_session_pool_->HasSession(pair_));

  // Check that the NetLog was filled reasonably.
  net::CapturingNetLog::CapturedEntryList entries;
  log.GetEntries(&entries);
  EXPECT_LT(0u, entries.size());

  // Check that we logged SPDY_SESSION_CLOSE correctly.
  int pos = net::ExpectLogContainsSomewhere(
      entries, 0,
      net::NetLog::TYPE_SPDY_SESSION_CLOSE,
      net::NetLog::PHASE_NONE);

  CapturingNetLog::CapturedEntry entry = entries[pos];
  int error_code = 0;
  ASSERT_TRUE(entry.GetNetErrorCode(&error_code));
  EXPECT_EQ(ERR_CONNECTION_CLOSED, error_code);
}

TEST_F(SpdySessionSpdy3Test, OutOfOrderSynStreams) {
  // Construct the request.
  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> req1(ConstructSpdyGet(NULL, 0, false, 1, HIGHEST));
  scoped_ptr<SpdyFrame> req2(ConstructSpdyGet(NULL, 0, false, 3, LOWEST));
  MockWrite writes[] = {
    CreateMockWrite(*req1, 2),
    CreateMockWrite(*req2, 1),
  };

  scoped_ptr<SpdyFrame> resp1(ConstructSpdyGetSynReply(NULL, 0, 1));
  scoped_ptr<SpdyFrame> body1(ConstructSpdyBodyFrame(1, true));
  scoped_ptr<SpdyFrame> resp2(ConstructSpdyGetSynReply(NULL, 0, 3));
  scoped_ptr<SpdyFrame> body2(ConstructSpdyBodyFrame(3, true));
  MockRead reads[] = {
    CreateMockRead(*resp1, 3),
    CreateMockRead(*body1, 4),
    CreateMockRead(*resp2, 5),
    CreateMockRead(*body2, 6),
    MockRead(ASYNC, 0, 7)  // EOF
  };

  session_deps_.host_resolver->set_synchronous_mode(true);

  StaticSocketDataProvider data(reads, arraysize(reads),
                                writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url, LOWEST, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_refptr<SpdyStream> spdy_stream2;
  TestCompletionCallback callback2;
  EXPECT_EQ(OK, session->CreateStream(url, HIGHEST, &spdy_stream2,
                                      BoundNetLog(), callback2.callback()));
  EXPECT_EQ(0u, spdy_stream2->stream_id());

  scoped_ptr<SpdyHeaderBlock> headers(new SpdyHeaderBlock);
  (*headers)[":method"] = "GET";
  (*headers)[":scheme"] = url.scheme();
  (*headers)[":host"] = url.host();
  (*headers)[":path"] = url.path();
  (*headers)[":version"] = "HTTP/1.1";
  scoped_ptr<SpdyHeaderBlock> headers2(new SpdyHeaderBlock);
  *headers2 = *headers;

  spdy_stream1->set_spdy_headers(headers.Pass());
  EXPECT_TRUE(spdy_stream1->HasUrl());

  spdy_stream2->set_spdy_headers(headers2.Pass());
  EXPECT_TRUE(spdy_stream2->HasUrl());

  spdy_stream1->SendRequest(false);
  spdy_stream2->SendRequest(false);
  MessageLoop::current()->RunUntilIdle();

  EXPECT_EQ(3u, spdy_stream1->stream_id());
  EXPECT_EQ(1u, spdy_stream2->stream_id());

  spdy_stream1->Cancel();
  spdy_stream1 = NULL;

  spdy_stream2->Cancel();
  spdy_stream2 = NULL;
}

TEST_F(SpdySessionSpdy3Test, CancelStream) {
  MockConnect connect_data(SYNCHRONOUS, OK);
  // Request 1, at HIGHEST priority, will be cancelled before it writes data.
  // Request 2, at LOWEST priority, will be a full request and will be id 1.
  scoped_ptr<SpdyFrame> req2(ConstructSpdyGet(NULL, 0, false, 1, LOWEST));
  MockWrite writes[] = {
    CreateMockWrite(*req2, 0),
  };

  scoped_ptr<SpdyFrame> resp2(ConstructSpdyGetSynReply(NULL, 0, 1));
  scoped_ptr<SpdyFrame> body2(ConstructSpdyBodyFrame(1, true));
  MockRead reads[] = {
    CreateMockRead(*resp2, 1),
    CreateMockRead(*body2, 2),
    MockRead(ASYNC, 0, 3)  // EOF
  };

  session_deps_.host_resolver->set_synchronous_mode(true);

  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url1, HIGHEST, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_refptr<SpdyStream> spdy_stream2;
  TestCompletionCallback callback2;
  GURL url2("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url2, LOWEST, &spdy_stream2,
                                      BoundNetLog(), callback2.callback()));
  EXPECT_EQ(0u, spdy_stream2->stream_id());

  scoped_ptr<SpdyHeaderBlock> headers(new SpdyHeaderBlock);
  (*headers)[":method"] = "GET";
  (*headers)[":scheme"] = url1.scheme();
  (*headers)[":host"] = url1.host();
  (*headers)[":path"] = url1.path();
  (*headers)[":version"] = "HTTP/1.1";
  scoped_ptr<SpdyHeaderBlock> headers2(new SpdyHeaderBlock);
  *headers2 = *headers;

  spdy_stream1->set_spdy_headers(headers.Pass());
  EXPECT_TRUE(spdy_stream1->HasUrl());

  spdy_stream2->set_spdy_headers(headers2.Pass());
  EXPECT_TRUE(spdy_stream2->HasUrl());

  spdy_stream1->SendRequest(false);
  spdy_stream2->SendRequest(false);

  EXPECT_EQ(0u, spdy_stream1->stream_id());

  spdy_stream1->Cancel();

  EXPECT_EQ(0u, spdy_stream1->stream_id());

  data.RunFor(1);

  EXPECT_EQ(0u, spdy_stream1->stream_id());
  EXPECT_EQ(1u, spdy_stream2->stream_id());

  spdy_stream1 = NULL;
  spdy_stream2->Cancel();
  spdy_stream2 = NULL;
}

TEST_F(SpdySessionSpdy3Test, CloseSessionWithTwoCreatedStreams) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  // Test that if a sesion is closed with two created streams pending,
  // it does not crash.  http://crbug.com/139518
  MockConnect connect_data(SYNCHRONOUS, OK);

  // No actual data will be sent.
  MockWrite writes[] = {
    MockWrite(ASYNC, 0, 1)  // EOF
  };

  MockRead reads[] = {
    MockRead(ASYNC, 0, 0)  // EOF
  };
  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url1, HIGHEST, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_refptr<SpdyStream> spdy_stream2;
  TestCompletionCallback callback2;
  GURL url2("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url2, LOWEST, &spdy_stream2,
                                      BoundNetLog(), callback2.callback()));
  EXPECT_EQ(0u, spdy_stream2->stream_id());

  scoped_ptr<SpdyHeaderBlock> headers(new SpdyHeaderBlock);
  (*headers)["method"] = "GET";
  (*headers)["scheme"] = url1.scheme();
  (*headers)["host"] = url1.host();
  (*headers)["url"] = url1.path();
  (*headers)["version"] = "HTTP/1.1";
  scoped_ptr<SpdyHeaderBlock> headers2(new SpdyHeaderBlock);
  *headers2 = *headers;

  spdy_stream1->set_spdy_headers(headers.Pass());
  EXPECT_TRUE(spdy_stream1->HasUrl());
  ClosingDelegate delegate1(spdy_stream1.get());
  spdy_stream1->SetDelegate(&delegate1);

  spdy_stream2->set_spdy_headers(headers2.Pass());
  EXPECT_TRUE(spdy_stream2->HasUrl());
  ClosingDelegate delegate2(spdy_stream2.get());
  spdy_stream2->SetDelegate(&delegate2);

  spdy_stream1->SendRequest(false);
  spdy_stream2->SendRequest(false);

  // Ensure that the streams have not yet been activated and assigned an id.
  EXPECT_EQ(0u, spdy_stream1->stream_id());
  EXPECT_EQ(0u, spdy_stream2->stream_id());

  // Ensure we don't crash while closing the session.
  session->CloseSessionOnError(ERR_ABORTED, true, "");

  EXPECT_TRUE(spdy_stream1->closed());
  EXPECT_TRUE(spdy_stream2->closed());

  spdy_stream1 = NULL;
  spdy_stream2 = NULL;
}

TEST_F(SpdySessionSpdy3Test, VerifyDomainAuthentication) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockConnect connect_data(SYNCHRONOUS, OK);

  // No actual data will be sent.
  MockWrite writes[] = {
    MockWrite(ASYNC, 0, 1)  // EOF
  };

  MockRead reads[] = {
    MockRead(ASYNC, 0, 0)  // EOF
  };
  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  // Load a cert that is valid for:
  //   www.example.org
  //   mail.example.org
  //   www.example.com
  base::FilePath certs_dir = GetTestCertsDirectory();
  scoped_refptr<X509Certificate> test_cert(
      ImportCertFromFile(certs_dir, "spdy_pooling.pem"));
  ASSERT_NE(static_cast<X509Certificate*>(NULL), test_cert);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  ssl.cert = test_cert;
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = GetSession(pair_);

  SSLConfig ssl_config;
  scoped_refptr<TransportSocketParams> transport_params(
      new TransportSocketParams(test_host_port_pair_,
                                MEDIUM,
                                false,
                                false,
                                OnHostResolutionCallback()));
  scoped_refptr<SOCKSSocketParams> socks_params;
  scoped_refptr<HttpProxySocketParams> http_proxy_params;
  scoped_refptr<SSLSocketParams> ssl_params(
      new SSLSocketParams(transport_params,
                          socks_params,
                          http_proxy_params,
                          ProxyServer::SCHEME_DIRECT,
                          test_host_port_pair_,
                          ssl_config,
                          0,
                          false,
                          false));
  scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
  EXPECT_EQ(OK, connection->Init(test_host_port_pair_.ToString(),
                                 ssl_params, MEDIUM, CompletionCallback(),
                                 http_session_->GetSSLSocketPool(
                                     HttpNetworkSession::NORMAL_SOCKET_POOL),
                                 BoundNetLog()));

  EXPECT_EQ(OK, session->InitializeWithSocket(connection.release(), false, OK));
  EXPECT_TRUE(session->VerifyDomainAuthentication("www.example.org"));
  EXPECT_TRUE(session->VerifyDomainAuthentication("mail.example.org"));
  EXPECT_TRUE(session->VerifyDomainAuthentication("mail.example.com"));
  EXPECT_FALSE(session->VerifyDomainAuthentication("mail.google.com"));
}

TEST_F(SpdySessionSpdy3Test, ConnectionPooledWithTlsChannelId) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockConnect connect_data(SYNCHRONOUS, OK);

  // No actual data will be sent.
  MockWrite writes[] = {
    MockWrite(ASYNC, 0, 1)  // EOF
  };

  MockRead reads[] = {
    MockRead(ASYNC, 0, 0)  // EOF
  };
  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  // Load a cert that is valid for:
  //   www.example.org
  //   mail.example.org
  //   www.example.com
  base::FilePath certs_dir = GetTestCertsDirectory();
  scoped_refptr<X509Certificate> test_cert(
      ImportCertFromFile(certs_dir, "spdy_pooling.pem"));
  ASSERT_NE(static_cast<X509Certificate*>(NULL), test_cert);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  ssl.channel_id_sent = true;
  ssl.cert = test_cert;
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = GetSession(pair_);

  SSLConfig ssl_config;
  scoped_refptr<TransportSocketParams> transport_params(
      new TransportSocketParams(test_host_port_pair_,
                                MEDIUM,
                                false,
                                false,
                                OnHostResolutionCallback()));
  scoped_refptr<SOCKSSocketParams> socks_params;
  scoped_refptr<HttpProxySocketParams> http_proxy_params;
  scoped_refptr<SSLSocketParams> ssl_params(
      new SSLSocketParams(transport_params,
                          socks_params,
                          http_proxy_params,
                          ProxyServer::SCHEME_DIRECT,
                          test_host_port_pair_,
                          ssl_config,
                          0,
                          false,
                          false));
  scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
  EXPECT_EQ(OK, connection->Init(test_host_port_pair_.ToString(),
                                 ssl_params, MEDIUM, CompletionCallback(),
                                 http_session_->GetSSLSocketPool(
                                     HttpNetworkSession::NORMAL_SOCKET_POOL),
                                 BoundNetLog()));

  EXPECT_EQ(OK, session->InitializeWithSocket(connection.release(), false, OK));
  EXPECT_TRUE(session->VerifyDomainAuthentication("www.example.org"));
  EXPECT_TRUE(session->VerifyDomainAuthentication("mail.example.org"));
  EXPECT_FALSE(session->VerifyDomainAuthentication("mail.example.com"));
  EXPECT_FALSE(session->VerifyDomainAuthentication("mail.google.com"));
}

TEST_F(SpdySessionSpdy3Test, CloseTwoStalledCreateStream) {
  // TODO(rtenneti): Define a helper class/methods and move the common code in
  // this file.
  MockConnect connect_data(SYNCHRONOUS, OK);

  SettingsMap new_settings;
  const SpdySettingsIds kSpdySettingsIds1 = SETTINGS_MAX_CONCURRENT_STREAMS;
  const uint32 max_concurrent_streams = 1;
  new_settings[kSpdySettingsIds1] =
      SettingsFlagsAndValue(SETTINGS_FLAG_NONE, max_concurrent_streams);

  scoped_ptr<SpdyFrame> req1(ConstructSpdyGet(NULL, 0, false, 1, LOWEST));
  scoped_ptr<SpdyFrame> req2(ConstructSpdyGet(NULL, 0, false, 3, LOWEST));
  scoped_ptr<SpdyFrame> req3(ConstructSpdyGet(NULL, 0, false, 5, LOWEST));
  MockWrite writes[] = {
    CreateMockWrite(*req1, 1),
    CreateMockWrite(*req2, 4),
    CreateMockWrite(*req3, 7),
  };

  // Set up the socket so we read a SETTINGS frame that sets max concurrent
  // streams to 1.
  scoped_ptr<SpdyFrame> settings_frame(ConstructSpdySettings(new_settings));

  scoped_ptr<SpdyFrame> resp1(ConstructSpdyGetSynReply(NULL, 0, 1));
  scoped_ptr<SpdyFrame> body1(ConstructSpdyBodyFrame(1, true));

  scoped_ptr<SpdyFrame> resp2(ConstructSpdyGetSynReply(NULL, 0, 3));
  scoped_ptr<SpdyFrame> body2(ConstructSpdyBodyFrame(3, true));

  scoped_ptr<SpdyFrame> resp3(ConstructSpdyGetSynReply(NULL, 0, 5));
  scoped_ptr<SpdyFrame> body3(ConstructSpdyBodyFrame(5, true));

  MockRead reads[] = {
    CreateMockRead(*settings_frame),
    CreateMockRead(*resp1, 2),
    CreateMockRead(*body1, 3),
    CreateMockRead(*resp2, 5),
    CreateMockRead(*body2, 6),
    CreateMockRead(*resp3, 8),
    CreateMockRead(*body3, 9),
    MockRead(ASYNC, 0, 10)  // EOF
  };

  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  // Read the settings frame.
  data.RunFor(1);

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url1, LOWEST, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));

  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_refptr<SpdyStream> spdy_stream2;
  TestCompletionCallback callback2;
  GURL url2("http://www.google.com");
  EXPECT_EQ(ERR_IO_PENDING,
            session->CreateStream(url2, LOWEST, &spdy_stream2,
                                  BoundNetLog(), callback2.callback()));

  scoped_refptr<SpdyStream> spdy_stream3;
  TestCompletionCallback callback3;
  GURL url3("http://www.google.com");
  EXPECT_EQ(ERR_IO_PENDING,
            session->CreateStream(url3, LOWEST, &spdy_stream3,
                                  BoundNetLog(), callback3.callback()));

  EXPECT_EQ(1u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(2u, session->pending_create_stream_queues(LOWEST));

  scoped_ptr<SpdyHeaderBlock> headers(new SpdyHeaderBlock);
  (*headers)[":method"] = "GET";
  (*headers)[":scheme"] = url1.scheme();
  (*headers)[":host"] = url1.host();
  (*headers)[":path"] = url1.path();
  (*headers)[":version"] = "HTTP/1.1";
  scoped_ptr<SpdyHeaderBlock> headers2(new SpdyHeaderBlock);
  *headers2 = *headers;
  scoped_ptr<SpdyHeaderBlock> headers3(new SpdyHeaderBlock);
  *headers3 = *headers;

  spdy_stream1->set_spdy_headers(headers.Pass());
  EXPECT_TRUE(spdy_stream1->HasUrl());
  spdy_stream1->SendRequest(false);

  // Run until 1st stream is closed.
  EXPECT_EQ(0u, spdy_stream1->stream_id());
  data.RunFor(3);
  EXPECT_EQ(1u, spdy_stream1->stream_id());
  EXPECT_EQ(1u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(1u, session->pending_create_stream_queues(LOWEST));

  EXPECT_TRUE(spdy_stream2.get() != NULL);
  spdy_stream2->set_spdy_headers(headers2.Pass());
  EXPECT_TRUE(spdy_stream2->HasUrl());
  spdy_stream2->SendRequest(false);

  // Run until 2nd stream is closed.
  EXPECT_EQ(0u, spdy_stream2->stream_id());
  data.RunFor(3);
  EXPECT_EQ(3u, spdy_stream2->stream_id());
  EXPECT_EQ(1u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(0u, session->pending_create_stream_queues(LOWEST));

  EXPECT_TRUE(spdy_stream3.get() != NULL);
  spdy_stream3->set_spdy_headers(headers3.Pass());
  EXPECT_TRUE(spdy_stream3->HasUrl());
  spdy_stream3->SendRequest(false);

  EXPECT_EQ(0u, spdy_stream3->stream_id());
  data.RunFor(4);
  EXPECT_EQ(5u, spdy_stream3->stream_id());
  EXPECT_EQ(0u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(0u, session->pending_create_stream_queues(LOWEST));
}

TEST_F(SpdySessionSpdy3Test, CancelTwoStalledCreateStream) {
  session_deps_.host_resolver->set_synchronous_mode(true);

  MockRead reads[] = {
    MockRead(SYNCHRONOUS, ERR_IO_PENDING)  // Stall forever.
  };

  StaticSocketDataProvider data(reads, arraysize(reads), NULL, 0);
  MockConnect connect_data(SYNCHRONOUS, OK);

  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  // Initialize the SpdySetting with 1 max concurrent streams.
  spdy_session_pool_->http_server_properties()->SetSpdySetting(
      test_host_port_pair_,
      SETTINGS_MAX_CONCURRENT_STREAMS,
      SETTINGS_FLAG_PLEASE_PERSIST,
      1);

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1("http://www.google.com");
  ASSERT_EQ(OK,
            session->CreateStream(url1, LOWEST, &spdy_stream1,
                                  BoundNetLog(), callback1.callback()));
  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_refptr<SpdyStream> spdy_stream2;
  TestCompletionCallback callback2;
  GURL url2("http://www.google.com");
  ASSERT_EQ(ERR_IO_PENDING,
            session->CreateStream(url2, LOWEST, &spdy_stream2,
                                  BoundNetLog(), callback2.callback()));

  scoped_refptr<SpdyStream> spdy_stream3;
  TestCompletionCallback callback3;
  GURL url3("http://www.google.com");
  ASSERT_EQ(ERR_IO_PENDING,
            session->CreateStream(url3, LOWEST, &spdy_stream3,
                                  BoundNetLog(), callback3.callback()));

  EXPECT_EQ(1u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(2u, session->pending_create_stream_queues(LOWEST));

  // Cancel the first stream, this will allow the second stream to be created.
  EXPECT_TRUE(spdy_stream1.get() != NULL);
  spdy_stream1->Cancel();
  spdy_stream1 = NULL;
  session->CancelPendingCreateStreams(&spdy_stream1);
  EXPECT_EQ(1u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(1u, session->pending_create_stream_queues(LOWEST));

  // Cancel the second stream, this will allow the third stream to be created.
  EXPECT_TRUE(spdy_stream2.get() != NULL);
  spdy_stream2->Cancel();
  spdy_stream2 = NULL;
  session->CancelPendingCreateStreams(&spdy_stream2);
  EXPECT_EQ(1u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(0u, session->pending_create_stream_queues(LOWEST));

  // Cancel the third stream.
  EXPECT_TRUE(spdy_stream3.get() != NULL);
  spdy_stream3->Cancel();
  spdy_stream3 = NULL;
  session->CancelPendingCreateStreams(&spdy_stream3);
  EXPECT_EQ(0u, session->num_active_streams() + session->num_created_streams());
  EXPECT_EQ(0u, session->pending_create_stream_queues(LOWEST));
}

TEST_F(SpdySessionSpdy3Test, NeedsCredentials) {
  MockConnect connect_data(SYNCHRONOUS, OK);
  MockRead reads[] = {
    MockRead(SYNCHRONOUS, ERR_IO_PENDING)  // Stall forever.
  };
  StaticSocketDataProvider data(reads, arraysize(reads), NULL, 0);
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  ssl.channel_id_sent = true;
  ssl.protocol_negotiated = kProtoSPDY3;
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  const GURL url("https:/www.foo.com");
  HostPortPair test_host_port_pair(url.host(), 443);
  HostPortProxyPair pair(test_host_port_pair, ProxyServer::Direct());

  scoped_refptr<SpdySession> session = GetSession(pair);

  SSLConfig ssl_config;
  scoped_refptr<TransportSocketParams> transport_params(
      new TransportSocketParams(test_host_port_pair,
                                MEDIUM,
                                false,
                                false,
                                OnHostResolutionCallback()));
  scoped_refptr<SOCKSSocketParams> socks_params;
  scoped_refptr<HttpProxySocketParams> http_proxy_params;
  scoped_refptr<SSLSocketParams> ssl_params(
      new SSLSocketParams(transport_params,
                          socks_params,
                          http_proxy_params,
                          ProxyServer::SCHEME_DIRECT,
                          test_host_port_pair,
                          ssl_config,
                          0,
                          false,
                          false));
  scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
  EXPECT_EQ(OK, connection->Init(test_host_port_pair.ToString(),
                                 ssl_params, MEDIUM, CompletionCallback(),
                                 http_session_->GetSSLSocketPool(
                                     HttpNetworkSession::NORMAL_SOCKET_POOL),
                                 BoundNetLog()));

  EXPECT_EQ(OK, session->InitializeWithSocket(connection.release(), true, OK));

  EXPECT_TRUE(session->NeedsCredentials());

  // Flush the SpdySession::OnReadComplete() task.
  MessageLoop::current()->RunUntilIdle();

  spdy_session_pool_->Remove(session);
}

TEST_F(SpdySessionSpdy3Test, SendCredentials) {
  MockConnect connect_data(SYNCHRONOUS, OK);
  MockRead reads[] = {
    MockRead(SYNCHRONOUS, ERR_IO_PENDING)  // Stall forever.
  };
  SettingsMap settings;
  scoped_ptr<SpdyFrame> settings_frame(ConstructSpdySettings(settings));
  MockWrite writes[] = {
    CreateMockWrite(*settings_frame),
  };
  StaticSocketDataProvider data(reads, arraysize(reads),
                                writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  ssl.channel_id_sent = true;
  ssl.protocol_negotiated = kProtoSPDY3;
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateNetworkSession();

  const GURL kTestUrl("https://www.foo.com");
  HostPortPair test_host_port_pair(kTestUrl.host(), 443);
  HostPortProxyPair pair(test_host_port_pair, ProxyServer::Direct());

  scoped_refptr<SpdySession> session = GetSession(pair);

  SSLConfig ssl_config;
  scoped_refptr<TransportSocketParams> transport_params(
      new TransportSocketParams(test_host_port_pair,
                                MEDIUM,
                                false,
                                false,
                                OnHostResolutionCallback()));
  scoped_refptr<SOCKSSocketParams> socks_params;
  scoped_refptr<HttpProxySocketParams> http_proxy_params;
  scoped_refptr<SSLSocketParams> ssl_params(
      new SSLSocketParams(transport_params,
                          socks_params,
                          http_proxy_params,
                          ProxyServer::SCHEME_DIRECT,
                          test_host_port_pair,
                          ssl_config,
                          0,
                          false,
                          false));
  scoped_ptr<ClientSocketHandle> connection(new ClientSocketHandle);
  EXPECT_EQ(OK, connection->Init(test_host_port_pair.ToString(),
                                 ssl_params, MEDIUM, CompletionCallback(),
                                 http_session_->GetSSLSocketPool(
                                     HttpNetworkSession::NORMAL_SOCKET_POOL),
                                 BoundNetLog()));

  EXPECT_EQ(OK, session->InitializeWithSocket(connection.release(), true, OK));
  EXPECT_TRUE(session->NeedsCredentials());

  // Flush the SpdySession::OnReadComplete() task.
  MessageLoop::current()->RunUntilIdle();

  spdy_session_pool_->Remove(session);
  EXPECT_FALSE(spdy_session_pool_->HasSession(pair));
}

TEST_F(SpdySessionSpdy3Test, UpdateStreamsSendWindowSize) {
  // Set SETTINGS_INITIAL_WINDOW_SIZE to a small number so that WINDOW_UPDATE
  // gets sent.
  SettingsMap new_settings;
  int32 window_size = 1;
  new_settings[SETTINGS_INITIAL_WINDOW_SIZE] =
      SettingsFlagsAndValue(SETTINGS_FLAG_NONE, window_size);

  // Set up the socket so we read a SETTINGS frame that sets
  // INITIAL_WINDOW_SIZE.
  MockConnect connect_data(SYNCHRONOUS, OK);
  scoped_ptr<SpdyFrame> settings_frame(ConstructSpdySettings(new_settings));
  MockRead reads[] = {
    CreateMockRead(*settings_frame, 0),
    MockRead(ASYNC, 0, 1)  // EOF
  };

  session_deps_.host_resolver->set_synchronous_mode(true);

  scoped_ptr<DeterministicSocketData> data(
      new DeterministicSocketData(reads, arraysize(reads), NULL, 0));
  data->set_connect_data(connect_data);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(data.get());

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();
  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  EXPECT_EQ(OK, session->CreateStream(test_url_, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  EXPECT_NE(spdy_stream1->send_window_size(), window_size);

  data->RunFor(1);  // Process the SETTINGS frame, but not the EOF
  MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(session->initial_send_window_size(), window_size);
  EXPECT_EQ(spdy_stream1->send_window_size(), window_size);

  // Release the first one, this will allow the second to be created.
  spdy_stream1->Cancel();
  spdy_stream1 = NULL;

  scoped_refptr<SpdyStream> spdy_stream2;
  EXPECT_EQ(OK, session->CreateStream(test_url_, MEDIUM, &spdy_stream2,
                                      BoundNetLog(), callback1.callback()));

  EXPECT_EQ(spdy_stream2->send_window_size(), window_size);
  spdy_stream2->Cancel();
  spdy_stream2 = NULL;
}

// Test that SpdySession::DoRead reads data from the socket without yielding.
// This test makes 32k - 1 bytes of data available on the socket for reading. It
// then verifies that it has read all the available data without yielding.
TEST_F(SpdySessionSpdy3Test, ReadDataWithoutYielding) {
  MockConnect connect_data(SYNCHRONOUS, OK);
  BufferedSpdyFramer framer(3, false);

  scoped_ptr<SpdyFrame> req1(ConstructSpdyGet(NULL, 0, false, 1, MEDIUM));
  MockWrite writes[] = {
    CreateMockWrite(*req1, 0),
  };

  // Build buffer of size kMaxReadBytes / 4 (-spdy_data_frame_size).
  ASSERT_EQ(32 * 1024, kMaxReadBytes);
  const int kPayloadSize = kMaxReadBytes / 4 - SpdyDataFrame::size();
  TestDataStream test_stream;
  scoped_refptr<net::IOBuffer> payload(new net::IOBuffer(kPayloadSize));
  char* payload_data = payload->data();
  test_stream.GetBytes(payload_data, kPayloadSize);

  scoped_ptr<SpdyFrame> partial_data_frame(
      framer.CreateDataFrame(1, payload_data, kPayloadSize, DATA_FLAG_NONE));
  scoped_ptr<SpdyFrame> finish_data_frame(
      framer.CreateDataFrame(1, payload_data, kPayloadSize - 1, DATA_FLAG_FIN));

  scoped_ptr<SpdyFrame> resp1(ConstructSpdyGetSynReply(NULL, 0, 1));

  // Write 1 byte less than kMaxReadBytes to check that DoRead reads up to 32k
  // bytes.
  MockRead reads[] = {
    CreateMockRead(*resp1, 1),
    CreateMockRead(*partial_data_frame, 2),
    CreateMockRead(*partial_data_frame, 3, SYNCHRONOUS),
    CreateMockRead(*partial_data_frame, 4, SYNCHRONOUS),
    CreateMockRead(*finish_data_frame, 5, SYNCHRONOUS),
    MockRead(ASYNC, 0, 6)  // EOF
  };

  // Create SpdySession and SpdyStream and send the request.
  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.host_resolver->set_synchronous_mode(true);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url1, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_ptr<SpdyHeaderBlock> headers(new SpdyHeaderBlock);
  (*headers)[":method"] = "GET";
  (*headers)[":scheme"] = url1.scheme();
  (*headers)[":host"] = url1.host();
  (*headers)[":path"] = url1.path();
  (*headers)[":version"] = "HTTP/1.1";

  spdy_stream1->set_spdy_headers(headers.Pass());
  EXPECT_TRUE(spdy_stream1->HasUrl());
  spdy_stream1->SendRequest(false);

  // Set up the TaskObserver to verify SpdySession::DoRead doesn't post a task.
  SpdySessionTestTaskObserver observer("spdy_session.cc", "DoRead");

  // Run until 1st read.
  EXPECT_EQ(0u, spdy_stream1->stream_id());
  data.RunFor(2);
  EXPECT_EQ(1u, spdy_stream1->stream_id());
  EXPECT_EQ(0u, observer.executed_count());

  // Read all the data and verify SpdySession::DoRead has not posted a task.
  data.RunFor(4);

  // Verify task observer's executed_count is zero, which indicates DoRead read
  // all the available data.
  EXPECT_EQ(0u, observer.executed_count());
  EXPECT_TRUE(data.at_write_eof());
  EXPECT_TRUE(data.at_read_eof());
}

// Test that SpdySession::DoRead yields while reading the data. This test makes
// 32k + 1 bytes of data available on the socket for reading. It then verifies
// that DoRead has yielded even though there is data available for it to read
// (i.e, socket()->Read didn't return ERR_IO_PENDING during socket reads).
TEST_F(SpdySessionSpdy3Test, TestYieldingDuringReadData) {
  MockConnect connect_data(SYNCHRONOUS, OK);
  BufferedSpdyFramer framer(3, false);

  scoped_ptr<SpdyFrame> req1(ConstructSpdyGet(NULL, 0, false, 1, MEDIUM));
  MockWrite writes[] = {
    CreateMockWrite(*req1, 0),
  };

  // Build buffer of size kMaxReadBytes / 4 (-spdy_data_frame_size).
  ASSERT_EQ(32 * 1024, kMaxReadBytes);
  const int kPayloadSize = kMaxReadBytes / 4 - SpdyDataFrame::size();
  TestDataStream test_stream;
  scoped_refptr<net::IOBuffer> payload(new net::IOBuffer(kPayloadSize));
  char* payload_data = payload->data();
  test_stream.GetBytes(payload_data, kPayloadSize);

  scoped_ptr<SpdyFrame> partial_data_frame(
      framer.CreateDataFrame(1, payload_data, kPayloadSize, DATA_FLAG_NONE));
  scoped_ptr<SpdyFrame> finish_data_frame(
      framer.CreateDataFrame(1, "h", 1, DATA_FLAG_FIN));

  scoped_ptr<SpdyFrame> resp1(ConstructSpdyGetSynReply(NULL, 0, 1));

  // Write 1 byte more than kMaxReadBytes to check that DoRead yields.
  MockRead reads[] = {
    CreateMockRead(*resp1, 1),
    CreateMockRead(*partial_data_frame, 2),
    CreateMockRead(*partial_data_frame, 3, SYNCHRONOUS),
    CreateMockRead(*partial_data_frame, 4, SYNCHRONOUS),
    CreateMockRead(*partial_data_frame, 5, SYNCHRONOUS),
    CreateMockRead(*finish_data_frame, 6, SYNCHRONOUS),
    MockRead(ASYNC, 0, 7)  // EOF
  };

  // Create SpdySession and SpdyStream and send the request.
  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.host_resolver->set_synchronous_mode(true);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url1, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_ptr<SpdyHeaderBlock> headers(new SpdyHeaderBlock);
  (*headers)[":method"] = "GET";
  (*headers)[":scheme"] = url1.scheme();
  (*headers)[":host"] = url1.host();
  (*headers)[":path"] = url1.path();
  (*headers)[":version"] = "HTTP/1.1";

  spdy_stream1->set_spdy_headers(headers.Pass());
  EXPECT_TRUE(spdy_stream1->HasUrl());
  spdy_stream1->SendRequest(false);

  // Set up the TaskObserver to verify SpdySession::DoRead posts a task.
  SpdySessionTestTaskObserver observer("spdy_session.cc", "DoRead");

  // Run until 1st read.
  EXPECT_EQ(0u, spdy_stream1->stream_id());
  data.RunFor(2);
  EXPECT_EQ(1u, spdy_stream1->stream_id());
  EXPECT_EQ(0u, observer.executed_count());

  // Read all the data and verify SpdySession::DoRead has posted a task.
  data.RunFor(6);

  // Verify task observer's executed_count is 1, which indicates DoRead has
  // posted only one task and thus yielded though there is data available for it
  // to read.
  EXPECT_EQ(1u, observer.executed_count());
  EXPECT_TRUE(data.at_write_eof());
  EXPECT_TRUE(data.at_read_eof());
}

// Test that SpdySession::DoRead yields while reading the data. This test makes
// 4 reads of kMaxReadBytes / 4 (-spdy_data_frame_size), one read of
// kMaxReadBytes (-spdy_data_frame_size), and some reads of kMaxReadBytes + 8k
// (-spdy_data_frame_size), bytes of data available on the socket for reading.
// It then verifies that DoRead has yielded even though there is data available
// for it to read (i.e, socket()->Read didn't return ERR_IO_PENDING during
// socket reads). Also verifies that SpdySession reads only
// SpdySession::kReadBufferSize data from the underlying transport.
// TODO(rtennti): Make this test work after fixes to DeterministicSocketData.
TEST_F(SpdySessionSpdy3Test, DISABLED_TestYieldingDuringLargeReadData) {
  MockConnect connect_data(SYNCHRONOUS, OK);
  BufferedSpdyFramer framer(3, false);

  scoped_ptr<SpdyFrame> req1(ConstructSpdyGet(NULL, 0, false, 1, MEDIUM));
  MockWrite writes[] = {
    CreateMockWrite(*req1, 0),
  };

  // Build buffer of size kMaxReadBytes / 4 (-spdy_data_frame_size).
  ASSERT_EQ(32 * 1024, kMaxReadBytes);
  TestDataStream test_stream;
  const int kSmallPayloadSize = kMaxReadBytes / 4 - SpdyDataFrame::size();
  scoped_refptr<net::IOBuffer> small_payload(
      new net::IOBuffer(kSmallPayloadSize));
  char* small_payload_data = small_payload->data();
  test_stream.GetBytes(small_payload_data, kSmallPayloadSize);

  // Build buffer of size kMaxReadBytes - (-spdy_data_frame_size).
  TestDataStream test_stream1;
  const int kMaxReadBytesPayloadSize = kMaxReadBytes - SpdyDataFrame::size();
  scoped_refptr<net::IOBuffer> max_bytes_payload(
      new net::IOBuffer(kMaxReadBytesPayloadSize));
  char* max_bytes_payload_data = max_bytes_payload->data();
  test_stream1.GetBytes(max_bytes_payload_data, kMaxReadBytesPayloadSize);

  // Build buffer of size kMaxReadBytes + kSmallPayloadSize
  // (-spdy_data_frame_size).
  TestDataStream test_stream2;
  const int kLargePayloadSize =
      kMaxReadBytes + kSmallPayloadSize - SpdyDataFrame::size();
  scoped_refptr<net::IOBuffer> large_payload(
      new net::IOBuffer(kLargePayloadSize));
  char* large_payload_data = large_payload->data();
  test_stream2.GetBytes(large_payload_data, kLargePayloadSize);

  scoped_ptr<SpdyFrame> small_data_frame(framer.CreateDataFrame(
      1, small_payload_data, kSmallPayloadSize, DATA_FLAG_NONE));
  scoped_ptr<SpdyFrame> max_bytes_data_frame(framer.CreateDataFrame(
      1, max_bytes_payload_data, kMaxReadBytesPayloadSize, DATA_FLAG_NONE));
  scoped_ptr<SpdyFrame> large_data_frame(framer.CreateDataFrame(
      1, large_payload_data, kLargePayloadSize, DATA_FLAG_NONE));
  scoped_ptr<SpdyFrame> finish_data_frame(framer.CreateDataFrame(
      1, "h", 1, DATA_FLAG_FIN));

  scoped_ptr<SpdyFrame> resp1(ConstructSpdyGetSynReply(NULL, 0, 1));

  MockRead reads[] = {
    CreateMockRead(*resp1, 1),
    CreateMockRead(*small_data_frame, 2),
    CreateMockRead(*small_data_frame, 3, SYNCHRONOUS),
    CreateMockRead(*small_data_frame, 4, SYNCHRONOUS),
    CreateMockRead(*small_data_frame, 5, SYNCHRONOUS),
    CreateMockRead(*max_bytes_data_frame, 6),
    CreateMockRead(*large_data_frame, 7, SYNCHRONOUS),
    CreateMockRead(*finish_data_frame, 8, SYNCHRONOUS),
    MockRead(ASYNC, 0, 9)  // EOF
  };

  // Create SpdySession and SpdyStream and send the request.
  DeterministicSocketData data(reads, arraysize(reads),
                               writes, arraysize(writes));
  data.set_connect_data(connect_data);
  session_deps_.host_resolver->set_synchronous_mode(true);
  session_deps_.deterministic_socket_factory->AddSocketDataProvider(&data);

  SSLSocketDataProvider ssl(SYNCHRONOUS, OK);
  session_deps_.deterministic_socket_factory->AddSSLSocketDataProvider(&ssl);

  CreateDeterministicNetworkSession();

  scoped_refptr<SpdySession> session = CreateInitializedSession();

  scoped_refptr<SpdyStream> spdy_stream1;
  TestCompletionCallback callback1;
  GURL url1("http://www.google.com");
  EXPECT_EQ(OK, session->CreateStream(url1, MEDIUM, &spdy_stream1,
                                      BoundNetLog(), callback1.callback()));
  EXPECT_EQ(0u, spdy_stream1->stream_id());

  scoped_ptr<SpdyHeaderBlock> headers(new SpdyHeaderBlock);
  (*headers)[":method"] = "GET";
  (*headers)[":scheme"] = url1.scheme();
  (*headers)[":host"] = url1.host();
  (*headers)[":path"] = url1.path();
  (*headers)[":version"] = "HTTP/1.1";

  spdy_stream1->set_spdy_headers(headers.Pass());
  EXPECT_TRUE(spdy_stream1->HasUrl());
  spdy_stream1->SendRequest(false);

  // Set up the TaskObserver to verify SpdySession::DoRead posts a task.
  SpdySessionTestTaskObserver observer("spdy_session.cc", "DoRead");

  // Run until 1st read.
  EXPECT_EQ(0u, spdy_stream1->stream_id());
  data.RunFor(2);
  EXPECT_EQ(1u, spdy_stream1->stream_id());
  EXPECT_EQ(0u, observer.executed_count());

  // Read all the data and verify SpdySession::DoRead has posted a task.
  data.RunFor(10);

  // Verify task observer's executed_count is 1, which indicates DoRead has
  // posted only one task and thus yielded though there is data available for
  // it to read.
  EXPECT_EQ(1u, observer.executed_count());
  EXPECT_TRUE(data.at_write_eof());
  EXPECT_TRUE(data.at_read_eof());
}

}  // namespace net

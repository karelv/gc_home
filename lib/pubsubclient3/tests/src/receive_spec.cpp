#include "BDDTest.h"
#include "Buffer.h"
#include "PubSubClient.h"
#include "ShimClient.h"
#include "trace.h"

// If this is changed to > 128 then the publish packet below
// is no longer valid as it assumes the remaining length
// is a single-byte. Don't make that mistake like I just
// did and lose a whole evening tracking down the issue.
#define PUBLISH_LEN 80

byte server[] = {172, 16, 0, 2};

bool callback_called = false;
char lastTopic[1024];
char lastPayload[1024];
size_t lastLength;

// function declarations
void callback(char* topic, uint8_t* payload, size_t plength);
void reset_callback();
int test_receive_callback();
int test_receive_stream();
int test_receive_max_sized_message();
int test_drop_invalid_remaining_length_message();
int test_receive_oversized_message();
int test_resize_buffer();
int test_receive_oversized_stream_message();
int test_receive_qos1();

void reset_callback() {
    callback_called = false;
    lastTopic[0] = '\0';
    lastPayload[0] = '\0';
    lastLength = 0;
}

void callback(char* topic, uint8_t* payload, size_t plength) {
    TRACE("Callback received topic=[" << topic << "] plength=" << plength << "\n")
    callback_called = true;
    strcpy(lastTopic, topic);
    memcpy(lastPayload, payload, plength);
    lastLength = plength;
}

int test_receive_callback() {
    IT("receives a callback message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30, 0xe, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};
    shimClient.respond(publish, 16);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic, "topic") == 0);
    IS_TRUE(memcmp(lastPayload, "payload", 7) == 0);
    IS_TRUE(lastLength == 7);

    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_stream() {
    IT("receives a streamed callback message");
    reset_callback();

    Stream stream;
    uint8_t payload[] = "payload";
    stream.expect(payload, sizeof(payload) - 1);

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient, stream);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30, 0xe, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};
    shimClient.respond(publish, 16);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic, "topic") == 0);
    IS_TRUE(lastLength == 7);

    IS_FALSE(stream.error());
    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_max_sized_message() {
    IT("receives an max-sized message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient);
    client.setBufferSize(PUBLISH_LEN);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30, PUBLISH_LEN - 2, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};
    byte bigPublish[PUBLISH_LEN];
    memset(bigPublish, 'A', PUBLISH_LEN);
    bigPublish[PUBLISH_LEN - 1] = 'B';
    memcpy(bigPublish, publish, 16);
    shimClient.respond(bigPublish, PUBLISH_LEN);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic, "topic") == 0);
    IS_TRUE(lastLength == PUBLISH_LEN - 9);
    IS_TRUE(memcmp(lastPayload, bigPublish + 9, lastLength) == 0);

    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_oversized_message() {
    IT("drops an oversized message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient);
    client.setBufferSize(PUBLISH_LEN - 1);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30, PUBLISH_LEN - 2, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};
    byte bigPublish[PUBLISH_LEN];
    memset(bigPublish, 'A', PUBLISH_LEN);
    bigPublish[PUBLISH_LEN - 1] = 'B';
    memcpy(bigPublish, publish, 16);
    shimClient.respond(bigPublish, PUBLISH_LEN);

    rc = client.loop();

    IS_TRUE(rc);

    IS_FALSE(callback_called);

    IS_FALSE(shimClient.error());

    END_IT
}

int test_drop_invalid_remaining_length_message() {
    IT("drops invalid remaining length message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30, 0x92, 0x92, 0x92, 0x92, 0x01, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};
    shimClient.respond(publish, 20);

    rc = client.loop();

    IS_FALSE(rc);

    IS_FALSE(callback_called);

    IS_FALSE(shimClient.error());

    END_IT
}

int test_resize_buffer() {
    IT("receives a message larger than the default maximum");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient);
    client.setBufferSize(PUBLISH_LEN - 1);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30, PUBLISH_LEN - 2, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};
    byte bigPublish[PUBLISH_LEN];
    memset(bigPublish, 'A', PUBLISH_LEN);
    bigPublish[PUBLISH_LEN - 1] = 'B';
    memcpy(bigPublish, publish, 16);
    // Send it twice
    shimClient.respond(bigPublish, PUBLISH_LEN);
    shimClient.respond(bigPublish, PUBLISH_LEN);

    rc = client.loop();
    IS_TRUE(rc);

    // First message fails as it is too big
    IS_FALSE(callback_called);

    // Resize the buffer
    client.setBufferSize(PUBLISH_LEN);

    rc = client.loop();
    IS_TRUE(rc);

    IS_TRUE(callback_called);

    IS_TRUE(strcmp(lastTopic, "topic") == 0);
    IS_TRUE(lastLength == PUBLISH_LEN - 9);
    IS_TRUE(memcmp(lastPayload, bigPublish + 9, lastLength) == 0);

    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_oversized_stream_message() {
    IT("receive an oversized streamed message");
    reset_callback();

    Stream stream;

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient, stream);
    client.setBufferSize(PUBLISH_LEN - 1);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30, PUBLISH_LEN - 2, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};

    byte bigPublish[PUBLISH_LEN];
    memset(bigPublish, 'A', PUBLISH_LEN);
    bigPublish[PUBLISH_LEN - 1] = 'B';
    memcpy(bigPublish, publish, 16);

    shimClient.respond(bigPublish, PUBLISH_LEN);
    stream.expect(bigPublish + 9, PUBLISH_LEN - 9);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic, "topic") == 0);

    IS_TRUE(lastLength == PUBLISH_LEN - 10);

    IS_FALSE(stream.error());
    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_qos1() {
    IT("receives a qos1 message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = {0x20, 0x02, 0x00, 0x00};
    shimClient.respond(connack, 4);

    PubSubClient client(server, 1883, callback, shimClient);
    bool rc = client.connect("client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x32, 0x10, 0x0, 0x5, 0x74, 0x6f, 0x70, 0x69, 0x63, 0x12, 0x34, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64};
    shimClient.respond(publish, 18);

    byte puback[] = {0x40, 0x2, 0x12, 0x34};
    shimClient.expect(puback, 4);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic, "topic") == 0);
    IS_TRUE(memcmp(lastPayload, "payload", 7) == 0);
    IS_TRUE(lastLength == 7);

    IS_FALSE(shimClient.error());

    END_IT
}

int main() {
    SUITE("Receive");
    test_receive_callback();
    test_receive_stream();
    test_receive_max_sized_message();
    test_drop_invalid_remaining_length_message();
    test_receive_oversized_message();
    test_resize_buffer();
    test_receive_oversized_stream_message();
    test_receive_qos1();

    FINISH
}

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

// Setup for RF24 network
RF24 radio(7, 8);  // CE, CSN pins
RF24Network network(radio);

// Constants
const unsigned long TRANSMISSION_INTERVAL = 600000;  // Time interval for transmission (in milliseconds)

// Timing variables
unsigned long startMillis = 0;

// Packet structure
struct packet_t {
    char type[5];     // Packet type
    char data1[10];   // Sensor data 1 ( soil moisture)
    char data2[10];   // Sensor data 2 ( temperature)
    char data3[10];   // Sensor data 3 ( PH)
    char data4[10];   // Sensor data 4 ( humidity)
};

// Sleep time structure
struct sleep_time_t {
    unsigned long sleepTime;  // Time until next cycle in milliseconds
};

// Node class representing each node in the network
class Node {
public:
    uint16_t id;
    Node* parent;
    Node** children;
    bool dataFlag;
    bool sleepFlag;

    // Constructor
    Node(uint16_t id) : id(id), parent(nullptr), children(nullptr), dataFlag(false), sleepFlag(false) {}

    // Check if the node is a leaf (has no children)
    bool isLeaf() {
        return children == nullptr;
    }

    // Count the number of children this node has
    int countChildren() {
        if (children == nullptr) {
            return 0;
        }
        int count = 0;
        while (children[count] != nullptr) {
            count++;
        }
        return count;
    }

    // Check if this node and all its children can sleep
    bool canSleep() {
        if (!dataFlag) {
            // If this node itself isn't ready to sleep, return false
            return false;
        }

        if (isLeaf()) {
            // If the node is a leaf and its flags are set, it can sleep
            return true;
        }

        // Check if all children are ready to sleep
        int numChildren = countChildren();
        for (int i = 0; i < numChildren; i++) {
            if (!children[i]->sleepFlag) {
                // If any child isn't ready to sleep, return false
                return false;
            }
        }
        return true;  // This node and all its children are ready to sleep
    }

    // Attempt to sleep the node
    void tryToSleep() {
        Serial.print("Trying to sleep Node: ");
        Serial.println(id, OCT);
        if (canSleep()) {
            calculateAndSendSleepTime();
            if (parent != nullptr) {
                parent->tryToSleep();  // Propagate the sleep attempt to the parent
            }
        }
    }

    // Calculate and send the sleep time
    bool calculateAndSendSleepTime() {
        unsigned long currentMillis = millis();
        unsigned long elapsedMillis = currentMillis - startMillis;
        unsigned long timeToNextCycle = TRANSMISSION_INTERVAL - (elapsedMillis % TRANSMISSION_INTERVAL);

        // Prepare the payload with the sleep time
        sleep_time_t sleepTime = { timeToNextCycle };
        RF24NetworkHeader header(id);
        bool success = network.write(header, &sleepTime, sizeof(sleepTime));

        if (!success) {
            Serial.print(F("Failed to send sleep time to Node "));
            Serial.println(id, OCT);
        } else {
            Serial.print(F("Sent sleep time to Node "));
            Serial.println(id, OCT);
        }
        return success;
    }

    // Reset both dataFlag and sleepFlag
    void resetFlags() {
        dataFlag = false;
        sleepFlag = false;
    }
};

// Global nodes (as an example)
Node* node1;
Node* node2;
Node* node3;

void setup() {
    Serial.begin(115200);
    SPI.begin();
    radio.begin();
    network.begin(90, 0);  // Channel and address for the master node

    startMillis = millis();

    // Setup nodes
    node1 = new Node(01);
    node2 = new Node(02);
    node3 = new Node(03);

    node1->children = new Node*[2];
    node1->children[0] = node2;
    node1->children[1] = node3;
    node2->parent = node1;
    node3->parent = node1;

    // Initialize flags as false (not ready to sleep initially)
    node1->resetFlags();
    node2->resetFlags();
    node3->resetFlags();
}

void loop() {
    network.update();  // Update network

    while (network.available()) {
        RF24NetworkHeader header;
        packet_t packet;
        network.read(header, &packet, sizeof(packet));

        Serial.print("Received packet from node ");
        Serial.println(header.from_node, OCT);

        // Mark the appropriate node's flags as true when receiving data
        Node* node = get(header.from_node);
        node->dataFlag = true;

        // Print the received data for debugging
        Serial.print("Data from node ");
        Serial.print(header.from_node, OCT);
        Serial.print(": ");
        Serial.print(packet.data1);  // soil moisture
        Serial.print(", ");
        Serial.print(packet.data2);  // temperature
        Serial.print(", ");
        Serial.print(packet.data3);  // PH
        Serial.print(", ");
        Serial.println(packet.data4); // humidity

        // After receiving data, try to sleep the node
        if (node != nullptr) {
            bool sleepSuccess = node->calculateAndSendSleepTime();
            if (sleepSuccess) {
                node->sleepFlag = true;  // Mark sleep flag true only if transmission is successful
            }
          else node->retry();
            get(header.from_node)->tryToSleep();  // Check if the parent node can sleep
        }
    }
}
 

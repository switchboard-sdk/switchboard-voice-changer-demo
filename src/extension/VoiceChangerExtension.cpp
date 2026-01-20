#include "extension/VoiceChangerExtension.hpp"
#include "extension/VoiceChangerNodeFactory.hpp"
#include "nodes/PitchShiftNode.hpp"
#include "nodes/RingModNode.hpp"

namespace voicechanger {

// NodeFactory implementation
VoiceChangerNodeFactory::VoiceChangerNodeFactory() {
    // Register PitchShiftNode
    registerNode(
        PitchShiftNode::getNodeTypeInfo(),
        [](const std::map<std::string, std::any>& config) -> switchboard::Node* {
            return new PitchShiftNode(config);
        }
    );

    // Register RingModNode
    registerNode(
        RingModNode::getNodeTypeInfo(),
        [](const std::map<std::string, std::any>& config) -> switchboard::Node* {
            return new RingModNode(config);
        }
    );
}

std::string VoiceChangerNodeFactory::getNodeTypePrefix() {
    return "VoiceChanger";
}

std::vector<switchboard::NodeTypeInfo> VoiceChangerNodeFactory::getNodeTypes() {
    return {
        PitchShiftNode::getNodeTypeInfo(),
        RingModNode::getNodeTypeInfo()
    };
}

// Extension implementation
VoiceChangerExtension::VoiceChangerExtension()
    : nodeFactory_(std::make_shared<VoiceChangerNodeFactory>()) {
}

std::string VoiceChangerExtension::getName() {
    return "VoiceChanger";
}

std::shared_ptr<switchboard::NodeFactory> VoiceChangerExtension::getNodeFactory() {
    return nodeFactory_;
}

switchboard::Result<void> VoiceChangerExtension::initialize(const std::map<std::string, std::any>& config) {
    (void)config;
    return switchboard::makeSuccess();
}

switchboard::Result<void> VoiceChangerExtension::deinitialize() {
    return switchboard::makeSuccess();
}

} // namespace voicechanger

// C API implementation
extern "C" {

switchboard::Extension* createExtension() {
    return new voicechanger::VoiceChangerExtension();
}

void destroyExtension(switchboard::Extension* extension) {
    delete extension;
}

}

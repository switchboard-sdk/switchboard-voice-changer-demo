#pragma once

#include <switchboard_core/NodeFactory.hpp>

#include <string>

namespace voicechanger {

/**
 * Node factory for VoiceChanger extension.
 * Creates PitchShiftNode, RingModNode, and other voice effect nodes.
 */
class VoiceChangerNodeFactory : public switchboard::NodeFactory {
public:
    VoiceChangerNodeFactory();
    ~VoiceChangerNodeFactory() override = default;

    std::string getNodeTypePrefix() override;
    std::vector<switchboard::NodeTypeInfo> getNodeTypes() override;
};

} // namespace voicechanger

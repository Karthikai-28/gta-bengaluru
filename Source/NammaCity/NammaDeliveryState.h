#pragma once

// Engine-independent rules: usable in automation and host-side tests.
enum class ENammaDeliveryStage { AwaitingPickup, Carrying, Complete };

struct FNammaDeliveryState
{
    ENammaDeliveryStage Stage = ENammaDeliveryStage::AwaitingPickup;
    bool PickUp()
    {
        if (Stage != ENammaDeliveryStage::AwaitingPickup) return false;
        Stage = ENammaDeliveryStage::Carrying;
        return true;
    }
    bool Deliver()
    {
        if (Stage != ENammaDeliveryStage::Carrying) return false;
        Stage = ENammaDeliveryStage::Complete;
        return true;
    }
    void Reset() { Stage = ENammaDeliveryStage::AwaitingPickup; }
};

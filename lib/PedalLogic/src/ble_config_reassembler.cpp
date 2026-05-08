#include "ble_config_reassembler.h"
#include "config_loader.h"

#include <Arduino.h>
#include <ArduinoJson.h>

static constexpr uint16_t END_SEQ = 0xFFFF;

enum class UploadCheck : std::uint8_t
{
    Ok,
    ParseFailed,   // JSON does not parse — maps to ERROR:parse_failed
    SchemaInvalid, // parses but violates structural contract — maps to ERROR:schema
};

// Schema pre-flight for an uploaded profiles.json. Rejects payloads the
// firmware would otherwise accept-then-misbehave on (TASK-233):
// missing/non-array `profiles`, or any profile entry without `buttons`.
// Loose by design — the CLI's pre-flight does full JSON-Schema validation.
static UploadCheck classifyUpload(const std::string& buffer)
{
    ArduinoJson::DynamicJsonDocument doc(kJsonDocCapacity);
    if (ArduinoJson::deserializeJson(doc, buffer))
    {
        return UploadCheck::ParseFailed;
    }

    if (! doc.containsKey("profiles") || ! doc["profiles"].is<ArduinoJson::JsonArray>())
    {
        return UploadCheck::SchemaInvalid;
    }

    for (ArduinoJson::JsonObject profile : doc["profiles"].as<ArduinoJson::JsonArray>())
    {
        if (! profile.containsKey("buttons") || ! profile["buttons"].is<ArduinoJson::JsonObject>())
        {
            return UploadCheck::SchemaInvalid;
        }
    }
    return UploadCheck::Ok;
}

BleConfigReassembler::BleConfigReassembler(ProfileManager* pm,
                                           IBleKeyboard* kb,
                                           IFileSystem* fs,
                                           ILogger* logger,
                                           std::vector<ILEDController*> selectLeds,
                                           StatusCallback statusCb,
                                           DelayFn delayFn,
                                           MillisFn millisFn,
                                           RebootFn rebootFn)
    : pm_(pm), kb_(kb), fs_(fs), logger_(logger), selectLeds_(std::move(selectLeds)),
      statusCb_(std::move(statusCb)), delay_(std::move(delayFn)), millis_(std::move(millisFn)),
      reboot_(std::move(rebootFn))
{
    if (! delay_)
    {
        delay_ = [](uint32_t ms) { delay(ms); };
    }
    if (! millis_)
    {
        millis_ = []() -> uint32_t { return static_cast<uint32_t>(millis()); };
    }
}

void BleConfigReassembler::onChunk(const uint8_t* data, size_t len, bool isHw)
{
    if (len < 2)
    {
        return;
    }

    auto seq = static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);

    if (seq == 0x0000)
    {
        if (transferInProgress_)
        {
            notifyStatus("BUSY");
            return;
        }
        transferInProgress_ = true;
        hwTransfer_ = isHw;
        nextExpectedSeq_ = 0;
        buffer_.clear();
    }

    if (! transferInProgress_)
    {
        return;
    }

    if (seq == END_SEQ)
    {
        if (hwTransfer_)
        {
            applyHwTransfer();
        }
        else
        {
            applyTransfer();
        }
        return;
    }

    if (seq != nextExpectedSeq_)
    {
        notifyStatus("ERROR:bad_sequence");
        resetTransfer();
        return;
    }

    size_t payloadLen = len - 2;
    if (buffer_.size() + payloadLen > kMaxConfigBytes)
    {
        notifyStatus("ERROR:too_large");
        resetTransfer();
        return;
    }

    buffer_.append(reinterpret_cast<const char*>(data + 2), payloadLen);
    nextExpectedSeq_++;
}

void BleConfigReassembler::applyTransfer()
{
    switch (classifyUpload(buffer_))
    {
        case UploadCheck::ParseFailed:
            notifyStatus("ERROR:parse_failed");
            blinkFailure();
            resetTransfer();
            return;
        case UploadCheck::SchemaInvalid:
            notifyStatus("ERROR:schema");
            blinkFailure();
            resetTransfer();
            return;
        case UploadCheck::Ok:
            break;
    }

    ConfigLoader loader(fs_, logger_);
    bool ok = loader.loadFromString(*pm_, kb_, buffer_);
    if (! ok)
    {
        // Schema check passed but loadFromString failed — most likely an
        // action-level value the loader rejects (e.g. unknown SendKey value).
        notifyStatus("ERROR:parse_failed");
        blinkFailure();
        resetTransfer();
        return;
    }
    // Persist the uploaded JSON verbatim rather than re-serialising from the
    // in-memory state. Round-trip via saveToFile is correct now (TASK-232
    // closed all action-serializer placeholders), but the buffer-write avoids
    // a redundant parse + serialise on a payload we have just validated.
    if (! fs_->writeFile("/profiles.json", buffer_))
    {
        notifyStatus("ERROR:write_failed");
        blinkFailure();
        resetTransfer();
        return;
    }
    blinkSuccess();
    notifyStatus("OK");
    resetTransfer();
}

void BleConfigReassembler::applyHwTransfer()
{
    if (! fs_->writeFile("/config.json", buffer_))
    {
        notifyStatus("ERROR:write_failed");
        blinkFailure();
        resetTransfer();
        return;
    }
    // Hardware-pin assignments are read once at firmware boot, so a CONFIG_WRITE_HW
    // upload only takes effect after a reboot. Notify the host first so the app's
    // UploadResult success path runs and the SnackBar shows, then give the BLE
    // stack a brief window to flush the notify before tearing the link down. The
    // blink doubles as that flush window. See TASK-273.
    notifyStatus("OK");
    blinkSuccess();
    resetTransfer();
    if (reboot_)
    {
        reboot_();
    }
}

void BleConfigReassembler::resetTransfer()
{
    transferInProgress_ = false;
    hwTransfer_ = false;
    nextExpectedSeq_ = 0;
    buffer_.clear();
}

void BleConfigReassembler::notifyStatus(const char* s)
{
    if (statusCb_)
    {
        statusCb_(s);
    }
}

void BleConfigReassembler::blinkSuccess()
{
    for (int i = 0; i < 3; i++)
    {
        for (auto* led : selectLeds_)
        {
            led->setState(true);
        }
        delay_(150);
        for (auto* led : selectLeds_)
        {
            led->setState(false);
        }
        delay_(150);
    }
    pm_->update(millis_());
}

void BleConfigReassembler::blinkFailure()
{
    for (auto* led : selectLeds_)
    {
        led->setState(true);
    }
    delay_(500);
    for (auto* led : selectLeds_)
    {
        led->setState(false);
    }
}

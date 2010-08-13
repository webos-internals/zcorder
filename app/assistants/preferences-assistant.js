function PreferencesAssistant() {
}

PreferencesAssistant.prototype.setup = function() {
    try {

    $('Recording Preferences').innerHTML=$L('Recording Preferences');

    this.optionChanged = this.optionChanged.bindAsEventListener(this);
    
    this.controller.setupWidget("stream_rate",
            {
                label: $L("Stream Rate"),
                labelPlacement: Mojo.Widget.labelPlacementLeft,
                choices: [
                    {label: $L("8000"), value: 8000},
                    {label: $L("11025"), value: 11025},
					{label: $L("12000"), value: 12000},
                    {label: $L("16000"), value: 16000}
                    ]
            },
            this.rateModel = {
                value: prefs.stream_rate,
                optionName: "stream_rate",
                disabled: false
            }
        );
    
    Mojo.Event.listen(this.controller.get("stream_rate"),
                      Mojo.Event.propertyChange, this.optionChanged);

    this.controller.setupWidget("lame_bitrate",
            {
                label: $L("Bitrate"),
                labelPlacement: Mojo.Widget.labelPlacementLeft,
                choices: [
                    {label: $L("96"), value: 96},
                    {label: $L("128"), value: 128}
                    ]
            },
            this.bitrateModel = {
                value: prefs.lame_bitrate,
                optionName: "lame_bitrate",
                disabled: false
            }
        );

    Mojo.Event.listen(this.controller.get("lame_bitrate"),
                      Mojo.Event.propertyChange, this.optionChanged);

    this.controller.setupWidget("lame_quality",
            {
                label: $L("Quality"),
                labelPlacement: Mojo.Widget.labelPlacementLeft,
                choices: [
                    {label: $L("Best"), value: 0},
					{label: $L("Medium"), value: 5},
                    {label: $L("Worst"), value: 9}
                    ]
            },
            this.qualityModel = {
                value: prefs.lame_quality,
                optionName: "lame_quality",
                disabled: false
            }
        );

    Mojo.Event.listen(this.controller.get("lame_quality"),
                      Mojo.Event.propertyChange, this.optionChanged);

   /* this.controller.setupWidget("voice_activation",
            {
                label: $L("Voice Activation (disabled)"),
                labelPlacement: Mojo.Widget.labelPlacementLeft,
                choices: [
                    {label: $L("On"), value: 0},
                    {label: $L("Off"), value: 0}
                    ]
            },
            this.vcvaModel = {
                value: prefs.voice_activation,
                optionName: "voice_activation",
                disabled: false
            }
        );

    Mojo.Event.listen(this.controller.get("voice_activation"),
                      Mojo.Event.propertyChange, this.optionChanged); */

    } catch(err) { Mojo.Log.error("err: %j", err); }
};

PreferencesAssistant.prototype.optionChanged = function(event) {
    prefs[event.model.optionName] = event.model.value;
    prefs.save();
};

PreferencesAssistant.prototype.activate = function(event) {
};

PreferencesAssistant.prototype.deactivate = function(event) {
};

PreferencesAssistant.prototype.cleanup = function(event) {
    Mojo.Event.stopListening(this.controller.get("stream_rate"),
        Mojo.Event.propertyChange, this.optionChanged);
    Mojo.Event.stopListening(this.controller.get("bitrate"),
        Mojo.Event.propertyChange, this.optionChanged);
    Mojo.Event.stopListening(this.controller.get("quality"),
        Mojo.Event.propertyChange, this.optionChanged);
 // Mojo.Event.stopListening(this.controller.get("voice_activation"),
 //     Mojo.Event.propertyChange, this.optionChanged);
};

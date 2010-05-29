function RecordingAssistant(){
}

RecordingAssistant.prototype.setup = function() {
	this.record = this.record.bind(this);
    this.stop = this.stop.bind(this);
	this.play = this.play.bind(this);
	this.recordSuccess = this.recordSuccess.bind(this);
	
	// appMenuModel defined in stage-assistant.js
	this.controller.setupWidget(Mojo.Menu.appMenu, {omitDefaultItems: true}, appMenuModel);
	
	// viewMenuModel defined in stage-assistant.js
	this.controller.setupWidget(Mojo.Menu.viewMenu, {spacerHeight: 0, menuClass:"no-fade"}, viewMenuModel);
	
	this.cmdMenuModel = {
    visible: true,
    items: [
        {items:[{label: $L("Record"), iconPath:'images/record.png', command:'record', disabled: false}, {label: $L("Stop"), iconPath:'images/stop.png', command:'stop', disabled: true}]},
        {items:[{label: $L("vox"), command:'toggle_vox', disabled: true}, {label: $L("play"), iconPath:'images/play.png', command:'play', disabled: true}]}
    ]
	};
	
	this.controller.setupWidget(Mojo.Menu.commandMenu, {}, this.cmdMenuModel);
};

RecordingAssistant.prototype.activate = function() {
	this.recordingStarted = this.recordingStarted.bind(this);
    this.recordFailure = this.recordFailure.bind(this);
    this.recordingStopped = this.recordingStopped.bind(this);
    this.stopFailure = this.stopFailure.bind(this);
	this.controller.serviceRequest('luna://org.webosinternals.zcorder', {
        	method: 'get_events',
			parameters: {
				subscribe: true
        	},
        	onSuccess: this.eventSuccess,
        	onFailure: this.eventFailure,
        	onError: this.eventFailure
    });
	this.resetPosition();
};

RecordingAssistant.prototype.resetPosition = function() {
	$("position").innerHTML = "0:00:00";
};

RecordingAssistant.prototype.eventSuccess = function(payload){
	if (payload.gst_message_type == 2) {
		$("error-messages").innerHTML = payload.message;
	}
	if (payload.gst_message_type == 4) {
		$("warning-messages").innerHTML = payload.message;
	}
	if (payload.time) {
		$("position").innerHTML = payload.time.substr(0,7);
	}
};

RecordingAssistant.prototype.eventFailure = function(response){
	$("error-messages").innerHTML = $L("Event subscription failed: ") + response.errorText;
};

RecordingAssistant.prototype.record = function(event) {
	this.recordingStarted();
    this.controller.serviceRequest('luna://org.webosinternals.zcorder', {
        method: 'start_record',
        parameters: {
            source_device: source_device,
            stream_rate: prefs.stream_rate,
            lame_bitrate: prefs.lame_bitrate,
            lame_quality: prefs.lame_quality,
            voice_activation: prefs.voice_activation,
            filename: filename
        },
		onSuccess: this.recordSuccess,
		onFailure: this.recordFailure,
        onError: this.recordFailure
    });
};

RecordingAssistant.prototype.recordSuccess = function(payload) {
	if (payload.lastfilename) {
		lastRecording = payload.lastfilename;
	}
	if (lastRecording) {
		this.cmdMenuModel.items[1].items[1].disabled = false; // play
		this.controller.modelChanged(this.cmdMenuModel);
	}
	currentRecording = false;
	this.cmdMenuModel.items[0].items[0].disabled = false; // record
	this.controller.modelChanged(this.cmdMenuModel);
	$("internal-messages").innerHTML = $L("Recording Stopped.<br>");
};

RecordingAssistant.prototype.recordingStarted = function(msg) {
    currentRecording = true;
	this.cmdMenuModel.items[1].items[1].disabled = true; // disable play button
	this.controller.modelChanged(this.cmdMenuModel);
	this.cmdMenuModel.items[0].items[0].disabled = true; // disable record button
    this.controller.modelChanged(this.cmdMenuModel);
    this.cmdMenuModel.items[0].items[1].disabled = false; // enable stop button
    this.controller.modelChanged(this.cmdMenuModel);
    $("internal-messages").innerHTML = $L("Recording...<br>");
};

RecordingAssistant.prototype.recordFailure = function(response) {
    currentRecording = false;
	this.cmdMenuModel.items[0].items[0].disabled = false; // record
    this.controller.modelChanged(this.cmdMenuModel);
    this.cmdMenuModel.items[0].items[1].disabled = true; // stop
    this.controller.modelChanged(this.cmdMenuModel);
    $("internal-messages").innerHTML = $L("Recording failed:<br>") + response.errorText;
};

RecordingAssistant.prototype.stop = function(event) {
	this.recordingStopping();
	this.controller.serviceRequest('luna://org.webosinternals.zcorder', {
		method: 'stop_record',
		onFailure: this.stopFailure,
		onError: this.stopFailure
	});
};

RecordingAssistant.prototype.recordingStopping = function(){
	this.cmdMenuModel.items[0].items[1].disabled = true; // stop
	this.controller.modelChanged(this.cmdMenuModel);
	$("internal-messages").innerHTML = $L("Saving, please wait...<br>");
};

RecordingAssistant.prototype.recordingStopped = function(response) {
};

RecordingAssistant.prototype.stopFailure = function(response) {
    currentRecording = false;  // Might as well, can't stop it now anyway
    this.cmdMenuModel.items[0].items[0].disabled = false; // record
	this.controller.modelChanged(this.cmdMenuModel);
    $("internal-messages").innerHTML = $L("Stop failed (WARNING, THIS SHOULD NEVER HAPPEN):<br>") + response.errorText;
};

RecordingAssistant.prototype.play = function(event) {
    p = {target: "file://" + lastRecording};
        
    this.controller.serviceRequest('palm://com.palm.applicationManager', {
        method: 'launch',
        parameters: {
            id: "com.palm.app.streamingmusicplayer",
            params: p
        }
    });
};

RecordingAssistant.prototype.deactivate = function(event) {
};

RecordingAssistant.prototype.cleanup = function(event) {
	if (currentRecording === true) {
			this.controller.serviceRequest('luna://org.webosinternals.zcorder', {
		method: 'stop_record'
		});
	}
};

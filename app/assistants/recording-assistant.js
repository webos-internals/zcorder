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
        {items:[{label: $L('Record'), iconPath:'images/record.png', command:'record', disabled: false}, {label: $L('Stop'), iconPath:'images/stop.png', command:'stop', disabled: true}]},
        {items:[{label: $L('vox'), command:'toggle_vox', disabled: true}, {label: $L('play'), iconPath:'images/play.png', command:'play', disabled: true}]}
    ]
	};
	
	this.controller.setupWidget(Mojo.Menu.commandMenu, {}, this.cmdMenuModel);
};

RecordingAssistant.prototype.activate = function() {
	this.recordingStarted = this.recordingStarted.bind(this);
    this.recordFailure = this.recordFailure.bind(this);
    this.recordingStopped = this.recordingStopped.bind(this);
    this.stopFailure = this.stopFailure.bind(this);
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
	if (payload.lastfilename) {
		lastRecording = payload.lastfilename;
		this.recordSuccess();
	}
	
};

RecordingAssistant.prototype.record = function(event) {
	this.recordingStarted();
    pluginelement.start_record(source_device, prefs.stream_rate, prefs.lame_bitrate, prefs.lame_quality, prefs.voice_activation, "filename");
};

RecordingAssistant.prototype.recordSuccess = function(payload) {
	if (lastRecording) {
		this.cmdMenuModel.items[1].items[1].disabled = false; // enable play
		this.controller.modelChanged(this.cmdMenuModel);
	}
	currentRecording = false;
	this.cmdMenuModel.items[0].items[0].disabled = false; // enable record
	this.controller.modelChanged(this.cmdMenuModel);
	$("internal-messages").innerHTML = "Recording Stopped.<br>";
};

RecordingAssistant.prototype.recordingStarted = function(msg) {
    currentRecording = true;
	this.cmdMenuModel.items[1].items[1].disabled = true; // disable play button
	this.controller.modelChanged(this.cmdMenuModel);
	this.cmdMenuModel.items[0].items[0].disabled = true; // disable record button
    this.controller.modelChanged(this.cmdMenuModel);
    this.cmdMenuModel.items[0].items[1].disabled = false; // enable stop button
    this.controller.modelChanged(this.cmdMenuModel);
    $("internal-messages").innerHTML = "Recording...<br>";
};

RecordingAssistant.prototype.recordFailure = function(response) {
    currentRecording = false;
	this.cmdMenuModel.items[0].items[0].disabled = false; // record
    this.controller.modelChanged(this.cmdMenuModel);
    this.cmdMenuModel.items[0].items[1].disabled = true; // stop
    this.controller.modelChanged(this.cmdMenuModel);
    $("internal-messages").innerHTML = "Recording failed:<br>" + response.errorText;
};

RecordingAssistant.prototype.stop = function(event) {
	this.recordingStopping();
	pluginelement.stop_record();
};

RecordingAssistant.prototype.recordingStopping = function(){
	this.cmdMenuModel.items[0].items[1].disabled = true; // disable stop
	this.controller.modelChanged(this.cmdMenuModel);
	$("internal-messages").innerHTML = "Saving, please wait...<br>";
};

RecordingAssistant.prototype.recordingStopped = function(response) {
};

RecordingAssistant.prototype.stopFailure = function(response) {
    currentRecording = false;  // Might as well, can't stop it now anyway
    this.cmdMenuModel.items[0].items[0].disabled = false; // record
	this.controller.modelChanged(this.cmdMenuModel);
    $("internal-messages").innerHTML = "Stop failed (WARNING, THIS SHOULD NEVER HAPPEN):<br>" + response.errorText;
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
		pluginelement.stop_record();
	}
};

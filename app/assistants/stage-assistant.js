function StageAssistant() {
	source_device = 0;
	filename = 0;
    currentRecording = false;
}

StageAssistant.prototype.setup = function() {
	appMenuModel = {
    	items: [
			{label: "Preferences", command: 'pushPrefs'},
            {label: "Support", command: 'pushSupport'}
    	]
	};
	
    prefs.load();
    this.controller.pushScene("recording");
	
	viewMenuModel = {
		visible: true,
		items: [
			{},
			{toggleCmd: 'mic',
			items: [
				{label: $L("Microphone"), command: 'mic', width: 160},
				{label: $L("Media Capture"), command: 'output', width: 160}
				] },
			{}
		]
	};
};
 
StageAssistant.prototype.handleCommand = function (event) {
    var currentScene = this.controller.activeScene();
	var stageController =  Mojo.Controller.appController.getActiveStageController();
	
	switch(event.type) {
        case Mojo.Event.commandEnable:
            switch (event.command) {
                case Mojo.Menu.prefsCmd:
                    if (!currentScene.assistant.prefsMenuDisabled) {
						event.stopPropagation();
					}
                    break;
                case Mojo.Menu.helpCmd:
                    if (!currentScene.assistant.helpMenuDisabled) {
						event.stopPropagation();
					}
                    break;
            }
            break;
        case Mojo.Event.command:
            switch (event.command) { 
				case 'mic':
					source_device = 0;
					break;
					
				case 'output':
					source_device = 1;
					break;
                
                case 'pushPrefs':
                    this.controller.pushScene('preferences');
                    break;
					
				case 'record':
					this.controller.delegateToSceneAssistant("record", "");
					break;
					
				case 'stop':
					this.controller.delegateToSceneAssistant("stop", "") ;
					break;
					
				case 'play':
					this.controller.delegateToSceneAssistant("play", "") ;
					break;
				
			    case Mojo.Menu.pushSupport:
                    this.controller.pushScene('support');
                    break;	
					
            }
        break;
    }
};

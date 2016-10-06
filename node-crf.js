var _nodecrf = require('./build/Release/nodecrf');

//Constructor
//All the properties besides the model are currently useless (I don't retrieve the advanced statistics)
var CRF = function(model, nbest, deepinfo) {

	this._crf = null;

	if (model === undefined) {
		throw new Error('A path to the model must be specified');
	}

	this.model = model;
	this.nbest = nbest || 2;
	this.deepinfo = deepinfo || true;

	this.isInitialized = false;

	//Separator used to separate the actual word from the POS tag
	this.separator = '_';
};

//Initialize the classficator
//It creates the actual C++ object
//Any changes performed after the invokation of this event are useless
CRF.prototype.init = function() {

	var command = '-m ' + this.model;

	if (this.nbest > 0) {
		command += ' -n' + this.nbest;
	}

	if (this.deepinfo) {
		command += ' -v 2';
	}

	this._crf = new _nodecrf.CRF(command);

	this.isInitialized = true;
};

//It peform the classification of the text
//The text must be previously POS tagged and in the form "WordSEPARATORTag WordSEPARATORTag"
//It return the category according to the trained model
CRF.prototype.classify = function(text) {

	if (!this.isInitialized) {
		throw new Error('Call the init() methods before classfying');
	}

	//TODO: regexp matching to verify the format of the text?

	var array = text.split(' ');

	for (var i = 0; i < array.length; i++) {
		array[i] = array[i].replace(this.separator, ' ');
	}

	var classification = this._crf.classify(array);

	return classification;

};

/**
 * Given a template and a trained model in CRF format,
 * the method trains the model and outputs it to a file.
 * @param crf_template_path The CRF template path.
 * @param crf_model_path The CRF training file path.
 * @param output_path The final output path.
 * @return 0 on success.
 */
CRF.learn = function(crf_template_path, crf_model_path, output_path) {
	var result = _nodecrf.learn(crf_template_path,crf_model_path,output_path);
	return result;
};

module.exports.CRF = CRF;

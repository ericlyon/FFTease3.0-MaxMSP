autowatch = 1;
var bufname = "richer";
var bufsize = 8192; // hard coded for fftz.enrich~

function set_buffer(x) {
	bufname = x;
}

function addsyn(){
	var tmp = [];
	var twopi = 8.0 * Math.atan(1.0);
	var i,j;
	var fbuf = new Buffer(bufname);
	// dc
	for(i = 0; i < bufsize; i++){
		tmp[i] = arguments[0];
	}	
	// harmonics
	for(j = 1; j < arguments.length; j++){
		for(i = 0; i < bufsize; i++){
			tmp[i] += Math.sin( (twopi * j * i) / bufsize);
		}
	}
	// fill the buffer
	fbuf.poke(1,0,tmp);
	// normalize
	fbuf.send("normalize",0.99);
}
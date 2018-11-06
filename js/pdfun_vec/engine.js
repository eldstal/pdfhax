var PDFUN_LOADED = false;

var pdfun = {}
pdfun.document = this

pdfun.entities = []

pdfun.alert = function() {
  app.alert("PDFUN engine loaded.", 3);
}

/* Clear any old annotations, etc */
pdfun.clear = function() {
  annots = this.document.getAnnots();
  if (annots) {
    for (a=0; a<annots.length; a++) {
      annots[a].destroy();
    }
  }
}


pdfun.entity = function(initializer) {
  ent = {};

  ent.sprite = this.document.addAnnot(initializer);
  ent.width = 1;
  ent.height = 1;
  ent.velocity = [ 0, 0 ];
  ent.pos = [ 0, 0 ];

  this.entities.push(ent)

  return ent;
}

pdfun.move = function(ent) {

  ent.pos[0] += ent.velocity[0];
  ent.pos[1] += ent.velocity[1];

  ent.sprite.rect = [ ent.pos[0],
                      ent.pos[1],
                      ent.pos[0] + ent.width,
                      ent.pos[1] + ent.height ];
}

pdfun.destroy = function(ent) {
  var index = this.entities.indexOf(ent);
  if (index > -1) {
    this.entities.splice(index, 1);
  }

  // Remove from the document as well
  ent.sprite.destroy();
}

pdfun.tick = null;
pdfun.main = function() {

  // Call the user's frame function
  if (this.tick) {
    this.tick();
  }

  // Move all objects
  for (e=0; e<this.entities.length; e++) {
    this.move(this.entities[e]);
  }
}

pdfun.ticker = app.setInterval("pdfun.main()", 20);

PDFUN_LOADED = true;

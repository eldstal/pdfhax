

// Crappy breakout clone using PDF Annotations


PAD_Y = 50;
PAD_W = 180;
PAD_H = 30;



while (!PDFUN_LOADED) {}


paddle = {};
paddle.sprite = this.addAnnot(
       { type: "Square",
         page: 0,
         rect: [0, PAD_Y, PAD_W, PAD_Y+PAD_H],
         fillColor: ["RGB", 0.2, 0.2, 0.25 ],
         strokeColor: ["RGB", 0.1, 0.1, 0.15]

       });
paddle.pos = 300;




dir = 1;
speed = 20;
function tick() {
  paddle.pos += dir * speed;
  if (paddle.pos < 0) speed *= -1;
  if (paddle.pos > (1280-PAD_W)) speed *= -1;

  paddle.sprite.rect = [ paddle.pos, PAD_Y, paddle.pos + PAD_W, PAD_Y + PAD_H ];

}

main_ticker = app.setInterval("tick()", 40);






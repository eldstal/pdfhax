PAD_Y = 50;
PAD_W = 180;
PAD_H = 30;

BRICK_W = 70;
BRICK_H = 20;


while (!PDFUN_LOADED) {}

pdfun.clear();

paddle = pdfun.entity(
         { type: "Square",
           page: 0,
           rect: [0, PAD_Y, PAD_W, PAD_Y+PAD_H],
           fillColor: ["RGB", 0.2, 0.2, 0.25 ],
           strokeColor: ["RGB", 0.1, 0.1, 0.15]
         });
paddle.width = PAD_W;
paddle.height = PAD_H;
paddle.pos = [ 0, PAD_Y ];
paddle.velocity = [ 20, 0 ];


/* TODO: Add bricks */


pdfun.tick = function () {
  if (paddle.pos[0] < 0) paddle.velocity[0] *= -1;
  if (paddle.pos[0] > (1280-PAD_W)) paddle.velocity[0] *= -1;
}




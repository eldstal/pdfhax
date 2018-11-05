console.println("3D Script loaded");

function before_frame(ev) {
  console.println("RenderEvent");
  ev.canvas.background.setColor(new Color(0.98, 0.82, 0.98), new Color(0.88, 0.72, 0.88));

  runtime.removeEventHandler(renderHandler);
}

renderHandler = new RenderEventHandler();
renderHandler.onEvent = before_frame;
runtime.addEventHandler(renderHandler);


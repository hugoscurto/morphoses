class RoboBall {
  
  float yaw, pitch, roll;
  float yOffset, pOffset, rOffset;

  RoboBall() {
  }

  float getYaw()   { return yaw; }
  float getAdjustedYaw()   { return yaw+yOffset; }
  void setYaw(float yaw) {
    this.yaw   = _wrap(yaw, -180, 180);
  }
  
  float getPitch() { return pitch; }
  float getAdjustedPitch()   { return pitch+pOffset; }
  void setPitch(float pitch) {
    this.pitch = _wrap(pitch, -180, 180);
  }
  
  float getRoll()  { return roll; }
  float getAdjustedRoll()   { return roll+rOffset; }
  void setRoll(float roll) {
    this.roll  = _wrap(roll, -180, 180);
  }
  
  float _wrap(float a, float min, float max) {
    float diff = max-min;
    while (a < min) a += diff;
    while (a > min) a -= diff;
    return a;
  }
  
  void syncOffsets() {
    yOffset = -yaw;
    yaw = 0;
    pOffset = -pitch;
    pitch = 0;
    rOffset = -roll;
    roll = 0;
  }
  
  void draw() {
    noStroke();

    pushMatrix();

    rotateY(radians(90));

    float c1 = cos(radians(-getAdjustedRoll()));
    float s1 = sin(radians(-getAdjustedRoll()));
    float c2 = cos(radians(getAdjustedPitch()));
    float s2 = sin(radians(getAdjustedPitch()));
    float c3 = cos(radians(-getAdjustedYaw()));
    float s3 = sin(radians(-getAdjustedYaw()));
    applyMatrix( c2*c3, s1*s3+c1*c3*s2, c3*s1*s2-c1*s3, 0,
                 -s2, c1*c2, c2*s1, 0,
                 c2*s3, c1*s2*s3-c3*s1, c1*c3+s1*s2*s3, 0,
                 0, 0, 0, 1);

    // Draw ball.
    strokeWeight(1);
    stroke(255, 255, 255);
    noFill();
    sphere(100);
    
    // Draw axis.
    strokeWeight(5);
    
    int ARROW_HEAD_LENGTH = 20;
    int ARROW_LENGTH      = 150;
    
    pushMatrix();
//    rotateY(radians(90));
    stroke(255, 0, 0);
    arrow(0, 0, ARROW_LENGTH, 0, ARROW_HEAD_LENGTH);
    popMatrix();
    
    pushMatrix();
    rotateY(radians(90));
    stroke(0, 255, 0);
    arrow(0, 0, ARROW_LENGTH, 0, ARROW_HEAD_LENGTH);
    popMatrix();

    pushMatrix();
    rotateZ(-radians(90));
    stroke(0, 0, 255);
    arrow(0, 0, ARROW_LENGTH, 0, ARROW_HEAD_LENGTH);    
    popMatrix();

    popMatrix();
  }
  
  void arrow(int x1, int y1, int x2, int y2, float len) {
    line(x1, y1, x2, y2);
    pushMatrix();
    translate(x2, y2);
    float a = atan2(x1-x2, y2-y1);
    rotate(a);
    line(0, 0, -len, -len);
    line(0, 0, len, -len);
    popMatrix();
  } 

  
  
}
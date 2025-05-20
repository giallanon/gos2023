### gl_Position
E' espressa in clip-space coordinate, che sono coordinate omogeneo (x,y,z,w)
Il cubo rappresentato da queste coordinate è un cubo (-w, -w, 0) - (w, w, 0)


(-w,-w,0)              (0,-w,0)
                          |   
                          |  /(0,0,w)
                          | /
                          |/
(-w,0,0)---------------(0,0,0)-------------->(w,0,0)
                          |
                          |
                          |
                          V
                       (0,w,0)                (w,w,0)


### NDC  (Normalized device coord)
NDC = clip-space-coord / w
viene fatto automaticamente da fragshader e riporta le coordinate omogenee di clip-space
in un cubo (-1,-1,0) - (1,1,1)


(-1,-1,0)              (0,-1,0)              (1,-1,0)
                          |   
                          |  /(0,0,1)
                          | /
                          |/
(-1,0,0)---------------(0,0,0)-------------->(1,0,0)
                          |
                          |
                          |
                          V
(-1,1,0)               (0,1,0)               (1,1,0)

la coordinata z va da 0 a 1, con 0=near e 1=far

### Screen space (deto anche FrameBuffer space)
dove:
	w = width del frame buffer (es: 1920)
	h = height del frame buffer (es: 1080)
	
La coordinata z va da 0 a 1

(0,0)                           (w,0) 
   ------------------------------->
   |
   |
   |
   |
   |
   |
   |
   V
(0,h)                           (w,h)

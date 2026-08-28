// https://www.shadertoy.com/view/Mtfyzl
// The MIT License
// Copyright © 2017 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Compressing normals by octahedral projection. The number on the top
// right is the number of bits used to store the hole normal, not each
// component. In this case anything above 18 bits looks pretty decent!
// See http://www.vis.uni-stuttgart.de/~engelhts/paper/vmvOctaMaps.pdf
//
// Compare to Fibonacci: https://www.shadertoy.com/view/4t2XWK
//

uint octahedral_encode (in vec3 nor, uint precision_num_bit)
{
    nor /= ( abs( nor.x ) + abs( nor.y ) + abs( nor.z ) );
    nor.xy = (nor.z >= 0.0) ? nor.xy : (1.0-abs(nor.yx))*sign(nor.xy);
    vec2 v = 0.5 + 0.5*nor.xy;

    uint mu = (1u << precision_num_bit)-1u;
    uvec2 d = uvec2(floor(v*float(mu)+0.5));
    return (d.y << precision_num_bit) | d.x;
}

vec3 octahedral_decode ( uint encoded_value, uint precision_num_bit )
{
    uint mu =(1u<<precision_num_bit)-1u;

    uvec2 d = uvec2( encoded_value, encoded_value >> precision_num_bit ) & mu;
    vec2 v = vec2(d)/float(mu);
    v = -1.0 + 2.0*v;

    // Rune Stubbe's version
    vec3 nor = vec3(v, 1.0 - abs(v.x) - abs(v.y));
    float t = max(-nor.z,0.0);
    nor.x += (nor.x>0.0)?-t:t;
    nor.y += (nor.y>0.0)?-t:t;

	return nor;
	//return normalize (nor);
}

#include "TTest.h"
#include "gosMath.h"
#include "gosGeomPos3.h"

using namespace gos;
namespace test_math
{
    //**************************************
    static constexpr f32 EPSILON = 1e-06f;

    static inline bool	vecAreVeryClose (const vec2f &a, const vec2f &b, f32 epsilon)	{ return (fabsf(a.x - b.x) <= epsilon && fabsf(a.y - b.y) <= epsilon); }
    static inline bool	vecAreVeryClose (const vec3f &a, const vec3f &b, f32 epsilon)	{ return (fabsf(a.x - b.x) <= epsilon && fabsf(a.y - b.y) <= epsilon && fabsf(a.z - b.z) <= epsilon); }
    static inline bool	vecAreVeryClose (const vec4f &a, const vec4f &b, f32 epsilon)	{ return (fabsf(a.x - b.x) <= epsilon && fabsf(a.y - b.y) <= epsilon && fabsf(a.z - b.z) <= epsilon && fabsf(a.w - b.w) <= epsilon); }
    static inline bool	mat33CompareVeryClose (const mat3x3f &a, const mat3x3f &b)
    {
        for (u8 r = 0; r < 3; r++)
        {
            for (u8 c = 0; c < 3; c++)
            {
                if (fabsf(a(r, c) - b(r, c)) > EPSILON)
                    return false;
            }
        }
        return true;
    }

    //********************************************
    int matrix1()
    {
        gos::vec2f		v2, retv2;
        gos::vec3f		v3,	retv3;
        gos::vec4f		v4, retv4;
        gos::mat2x2f	mat22;
        gos::mat3x3f	mat33, m33R1, m33R2;
        gos::mat4x4f	mat44;
        gos::mat3x4f	mat34;



        //row major / col major
        {
    #define ROWS	4
    #define COLS	3
            gos::math::Matrix<f32, true, ROWS, COLS> matCM;
            gos::math::Matrix<f32, false, ROWS, COLS> matRM;

            f32 v = 10;
            for (u8 r=0; r<ROWS; r++)
            {
                for (u8 c = 0; c < COLS; c++)
                {
                    matCM(r, c) = v;
                    matRM(r, c) = v;
                    v += 1;
                }
            }

            v = 10;
            for (u8 r=0; r<ROWS; r++)
            {
                for (u8 c = 0; c < COLS; c++)
                {
                    TEST_ASSERT(matCM(r, c) == matRM(r, c));
                    TEST_ASSERT(matCM(r, c) == v);
                    v += 1;
                }
            }


            u32 ct = 0;
            v = 10;
            for (u8 r=0; r<ROWS; r++)
            {
                for (u8 c = 0; c < COLS; c++)
                {
                    const f32 *p = matRM._getValuesPtConst();
                    TEST_ASSERT(p[ct++] == v);
                    v += 1;
                }
            }

            v = 10;
            for (u8 r=0; r<ROWS; r++)
            {
                for (u8 c = 0; c < COLS; c++)
                {
                    const f32 *p = matCM._getValuesPtConst();
                    TEST_ASSERT(p[c*ROWS +r] == v);
                    v += 1;
                }
            }

    #undef ROWS
    #undef COLS
        }




        //translation
        mat44.buildTranslation (5, 4, 3);
        mat34.buildTranslation (15, 14, 13);

        v3.set (0, 0, 0);		retv3 = math::vecTransform (mat44, v3); TEST_ASSERT(retv3 == vec3f(5, 4, 3));
        v3.set (0, 0, 0);		retv3 = math::vecTransform (mat34, v3); TEST_ASSERT(retv3 == vec3f(15, 14, 13));

        v4.set (0, 0, 0, 1);	retv4 = math::vecTransform (mat44, v4); TEST_ASSERT(retv4 == vec4f(5, 4, 3, 1));
        v4.set (0, 0, 0, 1);	retv3 = math::vecTransform (mat34, v4); TEST_ASSERT(retv3 == vec3f(15, 14, 13));

        //rotation (oraria, left hand system)
        /*
            y
            |    z
            |   /
            |  /
            | /
            |/
            |------------x
        */
        v2.set(1, 0);	 mat22.buildRotationMatrix (math::gradToRad(90));		retv2 = math::vecTransform (mat22, v2);	TEST_ASSERT(vecAreVeryClose(retv2, vec2f(0, 1), EPSILON));

        mat33.buildRotationAboutX(math::gradToRad(90));
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(1, 0, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, -1), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));

        mat33.buildRotationAboutY(math::gradToRad(90));
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, 1), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(-1, 0, 0), EPSILON));

        mat33.buildRotationAboutZ(math::gradToRad(90));
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, -1, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(1, 0, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, 1), EPSILON));

        //rotazione y90 + x90
        m33R1.buildRotationAboutY(math::gradToRad(90));
        m33R2.buildRotationAboutX(math::gradToRad(90));
        retv3 = math::vecTransform (m33R1, vec3f(1, 0, 0));
        retv3 = math::vecTransform (m33R2, retv3);
        TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));

        //ruoto prima per Y (R1) e poi per X (R2). Nota che le trasformazioni avvengono da DX vs SX
        mat33 = m33R2 * m33R1;
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, -1), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(-1, 0, 0), EPSILON));

        //inverto la rotazione, prima per X e poi per Y
        mat33 = m33R1 * m33R2;
        retv3 = math::vecTransform (mat33, vec3f(1,0,0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, 1), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0,1,0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(1, 0, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0,0,1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));

        //euler angle
        mat33.buildFromEulerAngles_YXZ (0, math::gradToRad(90), 0);
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(1, 0, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, -1), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));

        mat33.buildFromEulerAngles_YXZ (math::gradToRad(90), 0, 0);
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, 1), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(-1, 0, 0), EPSILON));

        mat33.buildFromEulerAngles_YXZ (0, 0, math::gradToRad(90));
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, -1, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(1, 0, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, 1), EPSILON));


        //rotazione euler y90 + x90  (prima 90 suy, poi 90 sull'attuale x)
        mat33.buildFromEulerAngles_YXZ (math::gradToRad(90), math::gradToRad(90), 0);
        retv3 = math::vecTransform (mat33, vec3f(1, 0, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 0, 1), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 1, 0));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(1, 0, 0), EPSILON));
        retv3 = math::vecTransform (mat33, vec3f(0, 0, 1));	TEST_ASSERT(vecAreVeryClose(retv3, vec3f(0, 1, 0), EPSILON));

        return 0;
    }

     //********************************************
    int quaterion()
    {
        gos::Quat	q, q1, q2;
        gos::mat3x3f mat33_1, mat33_2;
        gos::vec3f ax, ay, az;

        q.identity();	q.toMatrix3x3(&mat33_1);
        mat33_2.identity();
        TEST_ASSERT (mat33CompareVeryClose(mat33_1, mat33_2));

        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 0, 1), EPSILON));


        //rotazioni di base (oraria, left hand system)
        q.buildRotationAboutAsseX(math::gradToRad(90));
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(0, 0, -1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(1,0,0)), vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,1,0)), vec3f(0, 0, -1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,0,1)), vec3f(0, 1, 0), EPSILON));

        q.buildFromEuler_YXZ (0, math::gradToRad(90), 0);
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(0, 0, -1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(1,0,0)), vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,1,0)), vec3f(0, 0, -1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,0,1)), vec3f(0, 1, 0), EPSILON));


        q.buildRotationAboutAsseY(math::gradToRad(90));
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(-1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(1,0,0)), vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,1,0)), vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,0,1)), vec3f(-1, 0, 0), EPSILON));

        q.buildFromEuler_YXZ (math::gradToRad(90), 0, 0);
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(-1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(1,0,0)), vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,1,0)), vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,0,1)), vec3f(-1, 0, 0), EPSILON));



        q.buildRotationAboutAsseZ(math::gradToRad(90));
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(0, -1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(1,0,0)), vec3f(0, -1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,1,0)), vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,0,1)), vec3f(0, 0, 1), EPSILON));

        q.buildFromEuler_YXZ (0, 0, math::gradToRad(90));
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(0, -1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(1,0,0)), vec3f(0, -1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,1,0)), vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(math::vecTransform (q, vec3f(0,0,1)), vec3f(0, 0, 1), EPSILON));


        //prima 90y poi 90x (funziona come per le matrici, la moltiplicazione avviene da dx verso sinistra)
        //Se voglio prima ruotare Y e poi X, allora q = qX * qY
        q1.buildRotationAboutAsseY(math::gradToRad(90));
        q2.buildRotationAboutAsseX(math::gradToRad(90));
        q = q2 * q1;
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 1, 0), EPSILON));

        //ruoto 90y e poi ruoto l'attuale me stesso sull'asse 0,0,1
        q1.buildRotationAboutAsseY(math::gradToRad(90));
        q1.rotateMeAbout (vec3f(0,0,1), math::gradToRad(90));
        q1.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 1, 0), EPSILON));



        q.buildFromEuler_YXZ (math::gradToRad(90), math::gradToRad(90), 0);
        q.toAxis(&ax, &ay, &az);
        TEST_ASSERT(vecAreVeryClose(ax, vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(ay, vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(az, vec3f(0, 1, 0), EPSILON));

        return 0;
    }


    //********************************************
    int geompos()
    {
        gos::mat3x3f	mat33;
        gos::vec3f	v1, v2;
        geom::Pos3 pos;

        pos.identity();
        pos.moveRelAlongX (2); TEST_ASSERT(vecAreVeryClose(pos.o, vec3f(2, 0, 0), EPSILON));
        pos.moveRelAlongY (4); TEST_ASSERT(vecAreVeryClose(pos.o, vec3f(2, 4, 0), EPSILON));
        pos.moveRelAlongZ (-1); TEST_ASSERT(vecAreVeryClose(pos.o, vec3f(2, 4, -1), EPSILON));
        pos.warp (1, 0, 0); TEST_ASSERT(vecAreVeryClose(pos.o, vec3f(1, 0, 0), EPSILON));

        //rotation di base (oraria, left hand system)
        /*
            y
            |    z
            |   /
            |  /
            | /
            |/
            |------------x
        */
        pos.identity();	pos.rotateMeAboutMyX(math::gradToRad(90)); 
        TEST_ASSERT(vecAreVeryClose(pos.getAsseX(), vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(pos.getAsseY(), vec3f(0, 0, -1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(pos.getAsseZ(), vec3f(0, 1, 0), EPSILON));

        pos.identity();	pos.rotateMeAboutMyY(math::gradToRad(90)); 
        TEST_ASSERT(vecAreVeryClose(pos.getAsseX(), vec3f(0, 0, 1), EPSILON));
        TEST_ASSERT(vecAreVeryClose(pos.getAsseY(), vec3f(0, 1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(pos.getAsseZ(), vec3f(-1, 0, 0), EPSILON));

        pos.identity();	pos.rotateMeAboutMyZ(math::gradToRad(90)); 
        TEST_ASSERT(vecAreVeryClose(pos.getAsseX(), vec3f(0, -1, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(pos.getAsseY(), vec3f(1, 0, 0), EPSILON));
        TEST_ASSERT(vecAreVeryClose(pos.getAsseZ(), vec3f(0, 0, 1), EPSILON));



        //ruoto me stesso di 90° Y orario
        //il mio punto 1,0,0 locale, deve diventare il punto 0,0,1 in word coord
        //Viceversa, il punto wolrd 0,0,1, deve diventare 1,0,0 in locale
        pos.identity();	
        pos.rotateMeAboutMyY(math::gradToRad(90)); 
        v1.set (1, 0, 0);	pos.vect_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(0, 0, 1), EPSILON));
        v1.set (0, 0, 0);	pos.vect_ToLocal (&v2, &v1, 1);	TEST_ASSERT(vecAreVeryClose(v1, vec3f(1, 0, 0), EPSILON));

        pos.warp (3, 7, 9);
        v1.set (1, 0, 0);	pos.point_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(3, 7, 10), EPSILON));
        v1.set (0, 0, 0);	pos.point_ToLocal (&v2, &v1, 1);	TEST_ASSERT(vecAreVeryClose(v1, vec3f(1, 0, 0), EPSILON));
        v1.set (0, 0, 0);	pos.point_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, pos.o, EPSILON));
        v1.set (3, 7, 9);	pos.point_ToLocal (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(0, 0, 0), EPSILON));

        //rotazione y90 e a seguire una x90 sull'attuale asse X
        pos.identity();	
        pos.rotateMeAboutMyY(math::gradToRad(90)); 
        pos.rotateMeAboutMyX(math::gradToRad(90)); 
        v1.set (1, 0, 0);	pos.vect_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(0, 0, 1), EPSILON));
        v1.set (0, 1, 0);	pos.vect_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(1, 0, 0), EPSILON));
        v1.set (0, 0, 1);	pos.vect_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(0, 1, 0), EPSILON));

        //..e ora ruota sull'attuale asse Z	
        pos.rotateMeAboutMyZ(math::gradToRad(90));
        v1.set (1, 0, 0);	pos.vect_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(-1, 0, 0), EPSILON));
        v1.set (0, 1, 0);	pos.vect_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(0, 0, 1), EPSILON));
        v1.set (0, 0, 1);	pos.vect_ToWorld (&v1, &v2, 1);	TEST_ASSERT(vecAreVeryClose(v2, vec3f(0, 1, 0), EPSILON));

        return 0;
    }

} //namespace test_math


//*************************************
void testMath (Tester &tester)
{
	tester.run("math::matrix1", test_math::matrix1);
	tester.run("math::quaterion", test_math::quaterion);    
	tester.run("math::geompos", test_math::geompos);
}
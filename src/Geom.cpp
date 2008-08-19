// Geom.cpp

#include "stdafx.h"
#include "Geom.h"
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <GeomAPI_IntCS.hxx>
#include <GeomAPI_IntSS.hxx>

void ClosestPointsOnLines(const gp_Lin& lin, const gp_Lin& lin2, gp_Pnt &p1, gp_Pnt &p2)
{
	// they might be the same point
	gp_Vec v1(lin.Direction());
	gp_Vec v2(lin2.Direction());
	gp_Vec v3(lin2.Location(), lin.Location());

	double a = v1*v1;
	double b = v1*v2;
	double c = v2*v2;
	double d = v1*v3;
	double e = v2*v3;
	double D = a*c - b*b;
	double s, t;

    if (D < 0.00000000001){
		// the lines are almost parallel
        s = 0.0;
        t = (b>c ? d/b : e/c);
    }
    else {
        s = (b*e - c*d) / D;
        t = (a*e - b*d) / D;
    }

	p1 = lin.Location().XYZ() + s * v1.XYZ();
	p2 = lin2.Location().XYZ() + t * v2.XYZ();
}

bool intersect(const gp_Lin& lin, const gp_Lin& lin2, gp_Pnt &pnt)
{
	gp_Pnt p1, p2;
	ClosestPointsOnLines(lin, lin2, p1, p2);
	if(p1.IsEqual(p2, wxGetApp().m_geom_tol)){
		pnt = (p1.XYZ() + p2.XYZ()) /2;
		return true;
	}

	return false;
}

bool intersect(const gp_Lin& lin, const gp_Pln& pln, gp_Pnt &pnt)
{
	if(fabs(lin.Direction() * pln.Axis().Direction()) < 0.00000000000001)
		return false;
	double O = gp_Vec(pln.Location(), lin.Location()) * gp_Vec(pln.Axis().Direction());
	double sina = lin.Direction() * pln.Axis().Direction();
	double H = O/sina;
	pnt = lin.Location().XYZ() - lin.Direction().XYZ() * H;
	return true;
}

bool intersect(const gp_Pln& pln, const gp_Pln& pln2, gp_Lin& lin)
{
	bool result = false;
	{
		Handle (Geom_Plane) g_p = new Geom_Plane(pln);
		Handle (Geom_Plane) g_p2 = new Geom_Plane(pln2);

		{
			Handle (Geom_Surface) hs = Handle (Geom_Plane)::DownCast(g_p);
			Handle (Geom_Surface) hs2 = Handle (Geom_Plane)::DownCast(g_p2);

			{
				GeomAPI_IntSS intf(hs, hs2, wxGetApp().m_geom_tol);
				if(intf.IsDone()){
					int nl = intf.NbLines();
					if(nl>0){
						Handle (Geom_Curve) curve = intf.Line(1);
						lin = ((Handle(Geom_Line)::DownCast(curve)))->Lin();
						result = true;
					}
				}
			}
		}
	}

	return result;
}

bool intersect(const gp_Pnt& pnt, const gp_Pln& pln)
{
	double dist_from_plane = fabs(gp_Vec(pnt.XYZ()) * gp_Vec(pln.Axis().Direction()) - gp_Vec(pln.Location().XYZ()) * gp_Vec(pln.Axis().Direction()));
	if(dist_from_plane < wxGetApp().m_geom_tol)return true;
	return false;
}

bool intersect(const gp_Pnt& pnt, const gp_Lin& lin)
{
	gp_Pnt cp = ClosestPointOnLine(lin, pnt);
	return pnt.Distance(cp) <= wxGetApp().m_geom_tol;
}

bool intersect(const gp_Pnt& pnt, const gp_Circ& cir)
{
	gp_Pln pln(cir.Location(), cir.Axis().Direction());
	if(!intersect(pnt, pln))return false;

	return fabs(pnt.Distance(cir.Location()) - cir.Radius()) <= wxGetApp().m_geom_tol;
}

void intersect(const gp_Lin& line, const gp_Circ& circle, std::list<gp_Pnt> &list)
{
	std::list<gp_Pnt> plist;
	ClosestPointsLineAndCircle(line, circle, plist);
	for(std::list<gp_Pnt>::iterator It = plist.begin(); It != plist.end(); It++)
	{
		gp_Pnt& p = *It;
		if(intersect(p, line))list.push_back(p);
	}
}

void ClosestPointsLineAndCircle(const gp_Lin& line, const gp_Circ& circle, std::list<gp_Pnt> &list)
{
	// just do 2D equations, I can't work out the 3D case

	// the points will always be somewhere on the circle

	if(fabs(line.Direction() * circle.Axis().Direction()) > 0.4){
		// line is a bit perpendicular to plane of circle
		// just consider the point where the line passes through the plane of the circle

		gp_Pln pl(circle.Location(), circle.Axis().Direction());
		gp_Pnt plane_point;
		if(!intersect(line, pl, plane_point))return;

		double dist_to_centre = plane_point.Distance(circle.Location());

		if(dist_to_centre < wxGetApp().m_geom_tol)return;

		// just use the closest point in the direction of the closest point on line
		gp_Pnt p(circle.Location().XYZ() + gp_Vec(circle.Location(), plane_point).Normalized().XYZ() * circle.Radius());
		list.push_back(p);
	}
	else{
		// line is mostly flat in the plane of the circle

		gp_Pnt to_centre = ClosestPointOnLine(line, circle.Location());
		double dist_to_centre = to_centre.Distance(circle.Location());
		if(dist_to_centre > circle.Radius() - wxGetApp().m_geom_tol)
		{
			// just use the closest point in the direction of the closest point on line
			gp_Pnt p(circle.Location().XYZ() + (gp_Vec(circle.Location(), to_centre).Normalized() * circle.Radius()).XYZ());
			list.push_back(p);
		}
		else
		{

			// from geoff's geometry

			// solving	x = x0 + dx * t			x = y0 + dy * t
			//			x = xc + R * cos(a)		y = yc + R * sin(a)		for t
			// gives :-  t� (dx� + dy�) + 2t(dx*dx0 + dy*dy0) + (x0-xc)� + (y0-yc)� - R� = 0

			gp_Vec lv = line.Direction();

			gp_Dir x_axis = circle.XAxis().Direction();
			gp_Dir y_axis = circle.YAxis().Direction();
			gp_Pnt centre = circle.Location();

			// do the equation in the plane of the circle with it's centre being x0, y0
			gp_Vec local_line_location(circle.Location(), line.Location());
			double x0 = local_line_location * x_axis;
			double y0 = local_line_location * y_axis;

			// flatten line direction
			lv = lv - (lv*circle.Axis().Direction()) * circle.Axis().Direction();
			lv.Normalize();

			double dx = lv * x_axis;
			double dy = lv * y_axis;
			double R = circle.Radius();

			double a = dx * dx + dy * dy;
			double b = 2 * (dx* x0 + dy * y0);
			double c = x0 * x0 + y0 * y0 - R * R;

			// t = (-b +- sqrt(b*b - 4*a*c))/(2*a)
			double sq = sqrt(b*b - 4*a*c);
			double t1 = (-b + sq)/(2*a);
			double t2 = (-b - sq)/(2*a);

			gp_Pnt p1 = centre.XYZ() + x_axis.XYZ() * (x0 + t1 * dx) + y_axis.XYZ() * (y0 + t1 * dy);
			gp_Pnt p2 = centre.XYZ() + x_axis.XYZ() * (x0 + t2 * dx) + y_axis.XYZ() * (y0 + t2 * dy);

			list.push_back(p1);
			list.push_back(p2);
		}
	}
}

void intersect(const gp_Circ& c1, const gp_Circ& c2, std::list<gp_Pnt> &list)
{
	// from  http://local.wasp.uwa.edu.au/~pbourke/geometry/2circle/
	// a = (r0 * r0 - r1 * r1 + d * d ) / (2 * d)

	gp_Vec join(c1.Location(), c2.Location());
	double d = join.Magnitude();
	if(d < 0.00000000000001)return;
	double r0 = c1.Radius();
	double r1 = c2.Radius();
	if(r0 + r1 - d < -wxGetApp().m_geom_tol)return;// circles too far from each other

	gp_Vec forward = join.Normalized();
	gp_Vec left = c1.Axis().Direction() ^ join;

	if(r0 + r1 - d <= 0){
		list.push_back(c1.Location().XYZ() + forward.XYZ() * r0);
	}
	else{
		double a = (r0 * r0 - r1 * r1 + d * d ) / (2 * d);
		double h = sqrt(r0 * r0 - a * a);
		list.push_back(c1.Location().XYZ() + forward.XYZ() * a + left.XYZ() * h);
		list.push_back(c1.Location().XYZ() + forward.XYZ() * a - left.XYZ() * h);
	}
}

void extract(const gp_Pnt &p, double *m)
{
	m[0] = p.X();
	m[1] = p.Y();
	m[2] = p.Z();
}

void extract(const gp_Vec &v, double *m)
{
	m[0] = v.X();
	m[1] = v.Y();
	m[2] = v.Z();
}

void extract(const gp_XYZ &xyz, double *m)
{
	m[0] = xyz.X();
	m[1] = xyz.Y();
	m[2] = xyz.Z();
}

void extract(const gp_Trsf& tr, double *m)
{
	m[0] = tr.Value(1, 1);
	m[1] = tr.Value(1, 2);
	m[2] = tr.Value(1, 3);
	m[3] = tr.Value(1, 4);
	m[4] = tr.Value(2, 1);
	m[5] = tr.Value(2, 2);
	m[6] = tr.Value(2, 3);
	m[7] = tr.Value(2, 4);
	m[8] = tr.Value(3, 1);
	m[9] = tr.Value(3, 2);
	m[10] = tr.Value(3, 3);
	m[11] = tr.Value(3, 4);
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;
}

void extract_transposed(const gp_Trsf& tr, double *m)
{
	m[0] = tr.Value(1, 1);
	m[1] = tr.Value(2, 1);
	m[2] = tr.Value(3, 1);
	m[3] = 0;
	m[4] = tr.Value(1, 2);
	m[5] = tr.Value(2, 2);
	m[6] = tr.Value(3, 2);
	m[7] = 0;
	m[8] = tr.Value(1, 3);
	m[9] = tr.Value(2, 3);
	m[10] = tr.Value(3, 3);
	m[11] = 0;
	m[12] = tr.Value(1, 4);
	m[13] = tr.Value(2, 4);
	m[14] = tr.Value(3, 4);
	m[15] = 1;
}

gp_Pnt ClosestPointOnPlane(const gp_Pln& pln, const gp_Pnt &p){
	// returns closest vertex on plane to point pt 
	gp_Vec n = pln.Axis().Direction().XYZ();
	gp_Pnt l = pln.Location();

	gp_Pnt rp = p.XYZ() + (n * (n * gp_Vec(l.XYZ()) - n * gp_Vec(p.XYZ()))).XYZ();
	return rp;
}

gp_Pnt ClosestPointOnLine(const gp_Lin& line, const gp_Pnt &p){
	// returns closest vertex on line to point pt 

	double start_dotp = gp_Vec(line.Location().XYZ()) * line.Direction();
	double p_dotp = gp_Vec(p.XYZ()) * line.Direction();
	gp_Pnt close_pnt = line.Location().XYZ() + line.Direction().XYZ() * (p_dotp - start_dotp);

	return close_pnt;
}

gp_Pnt make_point(const double* p)
{
	return gp_Pnt(p[0], p[1], p[2]);
}

gp_Lin make_line(const gp_Pnt &p1, const gp_Pnt &p2)
{
    gp_Dir d(p2.XYZ() - p1.XYZ());
	return gp_Lin(p1, d);
}

gp_Lin make_line(const gp_Pnt &p, const gp_Vec &v)
{
    gp_Dir d(v.XYZ());
	return gp_Lin(p, d);
}

gp_Vec make_vector(const gp_Pnt &p1, const gp_Pnt &p2)
{
	return gp_Vec(p2.XYZ() - p1.XYZ());
}

gp_Vec make_vector(const double* v)
{
	return gp_Vec(v[0], v[1], v[2]);
}

gp_Trsf make_matrix(const double* m)
{
	gp_Trsf tr;
	tr.SetValues(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], 0.0001, 0.00000001);
	return tr;
}

gp_Trsf make_matrix(const gp_Pnt &origin, const gp_Vec &x_axis, const gp_Vec &y_axis)
{
	gp_Vec unit_x = x_axis.Normalized();

    double t = unit_x.X() * y_axis.X()
		+ unit_x.Y() * y_axis.Y()
		+ unit_x.Z() * y_axis.Z() ;
    gp_Vec y_orthogonal(y_axis.X() - unit_x.X() * t, y_axis.Y() - unit_x.Y() * t, y_axis.Z() - unit_x.Z() * t);

	gp_Vec unit_y = y_orthogonal.Normalized();
	gp_Vec unit_z = (unit_x ^ y_orthogonal).Normalized();

	double m[16] = {unit_x.X(), unit_y.X(), unit_z.X(), origin.X(), unit_x.Y(), unit_y.Y(), unit_z.Y(), origin.Y(), unit_x.Z(), unit_y.Z(), unit_z.Z(), origin.Z(), 0, 0, 0, 1};
	return make_matrix(m);
}

gp_Pnt transform_point(const gp_Trsf& tr, const gp_Pnt &p)
{
	gp_XYZ xyz = p.XYZ();
	tr.Transforms(xyz);
    return gp_Pnt(xyz);
}

gp_Vec transform_vector(const gp_Trsf& tr, const gp_Vec &v)
{
	gp_XYZ xyz = v.XYZ();
	tr.Transforms(xyz);
    return gp_Vec(xyz);
}

gp_Pln transform(const gp_Trsf& tr, const gp_Pln &p)
{
	return p.Transformed(tr);
}

void add_pnt_to_doubles(const gp_Pnt& pnt, std::list<double> &dlist)
{
	dlist.push_back(pnt.X());
	dlist.push_back(pnt.Y());
	dlist.push_back(pnt.Z());
}

int convert_pnts_to_doubles(const std::list<gp_Pnt> &plist, std::list<double> &dlist)
{
	for(std::list<gp_Pnt>::const_iterator It = plist.begin(); It != plist.end(); It++)
	{
		const gp_Pnt& pnt = *It;
		add_pnt_to_doubles(pnt, dlist);
	}

	return plist.size();
}

bool make_point_from_doubles(const std::list<double> &dlist, std::list<double>::const_iterator &It, gp_Pnt& pnt, bool four_doubles)
{
	if(It == dlist.end())return false;
	if(four_doubles)It++;
	pnt.SetX(*It);
	It++;
	pnt.SetY(*It);
	It++;
	pnt.SetZ(*It);
	It++;
	return true;
}

int convert_doubles_to_pnts(const std::list<double> &dlist, std::list<gp_Pnt> &plist, bool four_doubles)
{
	int nump = 0;
	for(std::list<double>::const_iterator It = dlist.begin(); It != dlist.end();)
	{
		gp_Pnt pnt;
		if(!make_point_from_doubles(dlist, It, pnt, four_doubles))break;
		plist.push_back(pnt);
		nump++;
	}
	return nump;
}
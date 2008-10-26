// Wire.cpp
#include "stdafx.h"
#include "Wire.h"
#include "../interface/Tool.h"
#include "BRepOffsetAPI_MakeOffset.hxx"

wxIcon* CWire::m_icon = NULL;

CWire::CWire(const TopoDS_Wire &wire, const wxChar* title):CShape(wire, title, false){
}

CWire::~CWire(){
}

wxIcon* CWire::GetIcon(){
	if(m_icon == NULL)
	{
		wxString exe_folder = wxGetApp().GetExeFolder();
		m_icon = new wxIcon(exe_folder + _T("/icons/wire.png"), wxBITMAP_TYPE_PNG);
	}
	return m_icon;
}

static wxString title_for_OffsetWireTool;

class OffsetWireTool:public Tool{
public:
	double m_offset;
	CWire* m_wire;

	OffsetWireTool(CWire* wire, double offset):m_wire(wire), m_offset(offset){}

	// Tool's virtual functions
	void Run(){
		BRepOffsetAPI_MakeOffset make_operation(m_wire->Wire());
		make_operation.Perform(m_offset);
		HeeksObj* new_object = CShape::MakeObject(make_operation.Shape(), _T("Result of Wire Offset"), SOLID_TYPE_UNKNOWN, HeeksColor(234, 123, 89));
		if(make_operation.Generated(make_operation.Shape()).Extent() > 0){
			wxMessageBox(_T("Generated"));
		}

		// ask about generation for each edge
		TopExp_Explorer ex;
		for ( ex.Init( m_wire->Shape(), TopAbs_EDGE ) ; ex.More(); ex.Next() )
		{
			TopoDS_Edge E = TopoDS::Edge(ex.Current());
			if(int extent = make_operation.Generated(E).Extent() > 0){
				wxChar mess[1024];
				wsprintf(mess, _T("Generated from edge = %d"), extent);
				wxMessageBox(mess);
			}
		}

		if(make_operation.Modified(make_operation.Shape()).Extent() > 0){
			wxMessageBox(_T("Modified"));
		}
		if(make_operation.IsDeleted(make_operation.Shape())){
			wxMessageBox(_T("IsDeleted"));
		}
		wxGetApp().AddUndoably(new_object, NULL, NULL);
		wxGetApp().DeleteUndoably(m_wire);
	}
	const wxChar* GetTitle(){
		wxChar str[1024];
		wsprintf(str, _T("Offset Wire ( %lf )"), m_offset);
		title_for_OffsetWireTool.assign(str);
		return title_for_OffsetWireTool.c_str();
	}
};

static OffsetWireTool offset_wire_out(NULL, 2.0);
static OffsetWireTool offset_wire_in(NULL, -2.0);

void CWire::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	offset_wire_out.m_wire = this;
	offset_wire_in.m_wire = this;
	t_list->push_back(&offset_wire_out);
	t_list->push_back(&offset_wire_in);
}

const TopoDS_Wire &CWire::Wire()const{
	return *((TopoDS_Wire*)(&m_shape));
}
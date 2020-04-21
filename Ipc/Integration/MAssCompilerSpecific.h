#pragma once

#ifdef _MSC_VER

// ��������� �����������
//
// ��������� ������ ������������ ��������������
// ���������� �������������� ��� ������

// �������� ��������� ���
#pragma warning ( 1 : 4189 )// 'identifier' : local variable is initialized but not referenced"

// ������� �� �������� ������ ����� ������� ������ �� �������, 
// � ��������� �������� ����� � �������� EAX
#pragma warning ( 1 : 4715 )
#pragma warning( error : 4715 )// : not all control paths return a value

// ������� �� �����
#pragma warning( error : 4717 )// : 'function' recursive on all control paths, function will cause runtime stack overflow

// �������� ���� ���������� ������ �������� ���� � ����� bool
// � ��������� �������� ������� ������������ int <- bool � bool <- NULL
#pragma warning ( 1 : 4800 )
#pragma warning( error : 4800 )// 'type' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning ( 1 : 4804 )
#pragma warning( error : 4804 )// 'operation' : unsafe use of type 'bool' in operation


// ������� �� �������� �������������� ������
#pragma warning( error : 4700 )// local variable 'name' used without having been initialized

// ������� �� ������ ������ � �������� ������.
#pragma warning( error : 4018 )// 'expression' : signed/unsigned mismatch

// ������������� ����
#pragma warning( error : 4541 )// 'identifier' used on polymorphic type 'type' with /GR-; unpredictable behavior may result

// �������� �� ���������� = ������ ==
// assignment within conditional expression
#pragma warning( 1: 4706 )
#pragma warning( error: 4706 )

// ����� ����� "class MainDlg : public CDialog, public AVBaseObject" �����������
// ���������� ��������� �� ���� ������� � MESSAGE_MAP ������ ��� ��������� �� 
// �������-���� � ����������� ������� �������, �� ����� ��� ������ �� �������-����
// � RefCounter ��� virtual CRefCounter. �������� ������ RefCounter->CRefCounter
// cast between different pointer to member representations, compiler may generate incorrect code
#pragma warning( 1: 4407 )
#pragma warning( error: 4407 )

#endif// _MSC_VER

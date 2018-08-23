#include "TraderApi.h"

#include "../../include/ApiHeader.h"
#include "../../include/ApiStruct.h"

#include "../../include/toolkit.h"

#include "../../include/queue/MsgQueue.h"

#include "../XTP/TypeConvert.h"

void* Query(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	// ���ڲ����ã����ü���Ƿ�Ϊ��
	CTraderApi* pApi = (CTraderApi*)pApi2;
	pApi->QueryInThread(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
	return nullptr;
}

CTraderApi::CTraderApi()
{
	m_pApi = nullptr;
	m_lRequestID = 0;
	m_nSleep = 1;

	// �Լ�ά��������Ϣ����
	m_msgQueue = new CMsgQueue();
	m_msgQueue_Query = new CMsgQueue();

	m_msgQueue_Query->Register(Query);
	m_msgQueue_Query->StartThread();
}
CTraderApi::~CTraderApi()
{
	_Disconnect(false);
}

void CTraderApi::Register(void* pCallback, void* pClass)
{
	m_pClass = pClass;
	if (m_msgQueue == nullptr)
		return;

	m_msgQueue_Query->Register(Query);
	m_msgQueue->Register((fnOnResponse)pCallback);
	if (pCallback)
	{
		m_msgQueue_Query->StartThread();
		m_msgQueue->StartThread();
	}
	else
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue->StopThread();
	}
}

void CTraderApi::Connect(const string& szPath,
	ServerInfoField* pServerInfo,
	UserInfoField* pUserInfo)
{
	m_szPath = szPath;
	memcpy(&m_ServerInfo, pServerInfo, sizeof(ServerInfoField));
	memcpy(&m_UserInfo, pUserInfo, sizeof(UserInfoField));

	m_msgQueue_Query->Input_NoCopy(RequestType::E_Init, m_msgQueue_Query, this, 0, 0,
		nullptr, 0, nullptr, 0, nullptr, 0);
}
void CTraderApi::Disconnect()
{
	_Disconnect(false);
}

void CTraderApi::ReqQuery(QueryType type, ReqQueryField* pQuery)
{
	m_msgQueue_Query->Input_Copy(type, m_msgQueue_Query, this, 0, 0,
		pQuery, sizeof(ReqQueryField), nullptr, 0, nullptr, 0);
}

char* CTraderApi::ReqOrderInsert(
	OrderField* pOrder,
	int count,
	char* pszLocalIDBuf)
{
	int OrderRef = -1;
	if (nullptr == m_pApi)
		return nullptr;

	XTPOrderInsertInfo body = { 0 };
	
	body.order_client_id = ++m_lRequestID;
	strncpy(body.ticker, pOrder->Symbol, sizeof(body.ticker));
	body.market = (XTP_MARKET_TYPE)Str_2_XTPMarketType(pOrder->ExchangeID);
	body.price = pOrder->Price;
	body.stop_price = pOrder->StopPx;
	body.quantity = pOrder->Qty;
	body.price_type = (XTP_PRICE_TYPE)OrderType_XAPI_2_XTP(pOrder->Type);
	body.side = (XTP_SIDE_TYPE)OrderDir_XAPI_2_XTP(pOrder->Side);
	body.position_effect = (XTP_POSITION_EFFECT_TYPE)OpenClose_XAPI_2_XTP(pOrder->OpenClose);

	body.business_type = XTP_BUSINESS_TYPE_CASH;

	m_pApi->InsertOrder(&body, m_session_id);
}

char* CTraderApi::ReqOrderAction(OrderIDType* szIds, int count, char* pzsRtn)
{
	unordered_map<string, XTPOrderInfo*>::iterator it = m_id_api_order.find(szIds[0]);
	if (it == m_id_api_order.end())
	{
		sprintf(pzsRtn, "%d", -100);
	}
	else
	{
		return ReqOrderAction(it->second, count, pzsRtn);
	}

	return pzsRtn;
}
char* CTraderApi::ReqOrderAction(XTPOrderInfo *pOrder, int count, char* pzsRtn)
{
	if (nullptr == m_pApi)
		return 0;

	m_pApi->CancelOrder(pOrder->order_xtp_id, m_session_id);

	strcpy(pzsRtn, "");
	return pzsRtn;
}

void CTraderApi::QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	int iRet = 0;
	switch (type)
	{
	case E_Init:
		iRet = _Init();
		break;
	case E_Disconnect:
		_Disconnect(true);
		// ����ѭ��
		return;
	}

	if (m_pApi)
	{
		switch (type)
		{
		case E_ReqUserLoginField:
			iRet = _ReqUserLogin(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
		case E_UserLogoutField:
			iRet = _ReqUserLogout(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
		case QueryType::QueryType_ReqQryTradingAccount:
			iRet = _ReqQryTradingAccount(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
		case QueryType::QueryType_ReqQryInvestorPosition:
			iRet = _ReqQryInvestorPosition(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
//		case QueryType::QueryType_ReqQryInstrument:
//			iRet = _ReqQryInstrument(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
//			break;
//		case QueryType::QueryType_ReqQryInvestor:
//			iRet = _ReqQryInvestor(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
//			break;
		case QueryType::QueryType_ReqQryOrder:
			iRet = _ReqQryOrder(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
		case QueryType::QueryType_ReqQryTrade:
			iRet = _ReqQryTrade(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;

#ifdef HAS_Quote
		case QueryType::QueryType_ReqQryQuote:
			iRet = _ReqQryQuote(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
#endif // HAS_Quote

#ifdef HAS_Settlement
		case E_SettlementInfoConfirmField:
			iRet = _ReqSettlementInfoConfirm(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
		case QueryType::QueryType_ReqQrySettlementInfo:
			iRet = _ReqQrySettlementInfo(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
			break;
#endif // HAS_Settlement

		default:
			break;
		}
	}

	if (0 == iRet)
	{
		//���سɹ�����ӵ��ѷ��ͳ�
		m_nSleep = 1;
	}
	else
	{
		m_msgQueue_Query->Input_Copy(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		//ʧ�ܣ���4���ݽ�����ʱ����������1s
		m_nSleep *= 4;
		m_nSleep %= 1023;
	}
	this_thread::sleep_for(chrono::milliseconds(m_nSleep));
}

void CTraderApi::Clear()
{
	for (unordered_map<string, OrderField*>::iterator it = m_id_platform_order.begin(); it != m_id_platform_order.end(); ++it)
		delete it->second;
	m_id_platform_order.clear();

	for (unordered_map<string, XTPOrderInfo*>::iterator it = m_id_api_order.begin(); it != m_id_api_order.end(); ++it)
		delete it->second;
	m_id_api_order.clear();

	for (unordered_map<string, PositionField*>::iterator it = m_id_platform_position.begin(); it != m_id_platform_position.end(); ++it)
		delete it->second;
	m_id_platform_position.clear();

	for (unordered_map<string, XTPQueryStkPositionRsp*>::iterator it = m_id_api_position.begin(); it != m_id_api_position.end(); ++it)
		delete it->second;
	m_id_api_position.clear();

}

int CTraderApi::_Init()
{
	m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Initialized, 0, nullptr, 0, nullptr, 0, nullptr, 0);

	m_pApi = XTP::API::TraderApi::CreateTraderApi(m_UserInfo.ExtInfoInt32, m_szPath.c_str());

	if (m_pApi)
	{
		m_pApi->RegisterSpi(this);

		// ��ʼ����������
		m_pApi->SetHeartBeatInterval(15);
		m_pApi->SubscribePublicTopic(XTP_TERT_QUICK);

		// ����XTP key����XTP���뿪��ʱ����xtp��Ա�ṩ
		m_pApi->SetSoftwareKey(m_UserInfo.ExtInfoChar64);

		// XTPû�е���������ǰ�ýӿڣ���������ģ��ǰ�û����ӳɹ���Ϣ
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Connecting, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Connected, 0, nullptr, 0, nullptr, 0, nullptr, 0);

		//���ӳɹ����Զ������¼
		ReqUserLogin();
	}

	return 0;
}
void CTraderApi::_Disconnect(bool IsInQueue)
{
	if (IsInQueue)
	{}
	else
	{
		if (m_msgQueue_Query)
		{
			m_msgQueue_Query->StopThread();
			m_msgQueue_Query->Register(nullptr);
			m_msgQueue_Query->Clear();
			delete m_msgQueue_Query;
			m_msgQueue_Query = nullptr;
		}
	}

	if (m_pApi)
	{
		// Ϊ�θо�����û�еǳ��ɹ����������У�
		m_pApi->RegisterSpi(nullptr);
		m_pApi->Release();
		m_pApi = nullptr;

		// ȫ����ֻ�����һ��
		m_msgQueue->Clear();
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Disconnected, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		// ��������
		m_msgQueue->Process();
	}

	if (m_msgQueue)
	{
		m_msgQueue->StopThread();
		m_msgQueue->Register(nullptr);
		m_msgQueue->Clear();
		delete m_msgQueue;
		m_msgQueue = nullptr;
	}

	m_lRequestID = 0;

	Clear();
}
void CTraderApi::_DisconnectInThread()
{
	m_msgQueue_Query->Input_NoCopy(RequestType::E_Disconnect, m_msgQueue_Query, this, 0, 0,
		nullptr, 0, nullptr, 0, nullptr, 0);
}

void CTraderApi::ReqUserLogin()
{
	m_msgQueue_Query->Input_NoCopy(RequestType::E_ReqUserLoginField, m_msgQueue_Query, this, 0, 0,
		nullptr, 0, nullptr, 0, nullptr, 0);
}
int CTraderApi::_ReqUserLogin(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Logining, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	auto tmp_session = m_pApi->Login(m_ServerInfo.Address, m_ServerInfo.Port, m_UserInfo.UserID, m_UserInfo.Password, XTP_PROTOCOL_TCP);
	if (m_session_id == 0)
	{
		m_session_id = tmp_session;
	}

	if (tmp_session > 0)
	{
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Logined, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Done, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	}
	else
	{
		m_msgQueue->Input_NoCopy(ResponseType::ResponseType_OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::ConnectionStatus_Disconnected, 0, nullptr, 0, nullptr, 0, nullptr, 0);
		_DisconnectInThread();
	}

	return 0;
}

void CTraderApi::ReqUserLogout()
{
	uint64_t* session = (uint64_t*)m_msgQueue_Query->new_block(sizeof(uint64_t));
	*session = m_session_id;

	if (m_msgQueue_Query)
	{
		m_msgQueue_Query->Input_NoCopy(RequestType::E_UserLogoutField, m_msgQueue_Query, this, 0, 0,
			session, sizeof(uint64_t), nullptr, 0, nullptr, 0);
	}
}
int CTraderApi::_ReqUserLogout(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	if (m_pApi)
	{
		return m_pApi->Logout(*(uint64_t*)pApi1);
	}
	return 0;
}

int CTraderApi::_ReqQryTradingAccount(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	if (m_pApi)
	{
		return m_pApi->QueryAsset(m_session_id, ++m_lRequestID);
	}
	return 0;
}
int CTraderApi::_ReqQryInvestorPosition(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	if (m_pApi)
	{
		ReqQueryField *query = (ReqQueryField*)ptr1;
		return m_pApi->QueryPosition(query->Symbol, m_session_id, ++m_lRequestID);
	}
	return 0;
}
int CTraderApi::_ReqQryOrder(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	if (m_pApi == nullptr)
	{
		return 0;
	}

	ReqQueryField *query = (ReqQueryField*)ptr1;

	XTPQueryOrderReq xtp_query = { 0 };
	strncpy(xtp_query.ticker, query->Symbol, sizeof(xtp_query.ticker));

	if (query->DateStart == 0)
	{
		xtp_query.begin_time = 0;
	}
	else
	{
		xtp_query.begin_time = query->DateStart * 1000000000;
	}

	if (query->DateEnd == 0)
	{
		xtp_query.end_time = 0;
	}
	else
	{
		xtp_query.end_time = query->DateEnd * 1000000000;
	}

	return m_pApi->QueryOrders(&xtp_query, m_session_id, ++m_lRequestID);
}
int CTraderApi::_ReqQryTrade(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	if (m_pApi == nullptr)
	{
		return 0;
	}

	ReqQueryField *query = (ReqQueryField*)ptr1;
	XTPQueryTraderReq xtp_query = { 0 };
	strncpy(xtp_query.ticker, query->Symbol, sizeof(xtp_query.ticker));

	if (query->DateStart == 0)
	{
		xtp_query.begin_time = 0;
	}
	else
	{
		xtp_query.begin_time = query->DateStart * 1000000000;
	}

	if (query->DateEnd == 0)
	{
		xtp_query.end_time = 0;
	}
	else
	{
		xtp_query.end_time = query->DateEnd * 1000000000;
	}

	return m_pApi->QueryTrades(&xtp_query, m_session_id, ++m_lRequestID);
}

void CTraderApi::OnTrade(TradeField *pTrade)
{
	PositionIDType positionId = { 0 };
	sprintf(positionId, "%s:%s:%d",
		pTrade->ExchangeID, pTrade->Symbol, TradeField_2_PositionSide(pTrade));

	PositionField* pField = nullptr;
	unordered_map<string, PositionField*>::iterator it = m_id_platform_position.find(positionId);
	if (it == m_id_platform_position.end())
	{
		pField = (PositionField*)m_msgQueue->new_block(sizeof(PositionField));

		strcpy(pField->Symbol, pTrade->InstrumentID);
		strcpy(pField->InstrumentID, pTrade->InstrumentID);
		strcpy(pField->ExchangeID, pTrade->ExchangeID);
		strcpy(pField->AccountID, pTrade->AccountID);
		pField->Side = (PositionSide)TradeField_2_PositionSide(pTrade);
		pField->HedgeFlag = pTrade->HedgeFlag;

		m_id_platform_position.insert(pair<string, PositionField*>(positionId, pField));
	}
	else
	{
		pField = it->second;
	}

	if (pTrade->OpenClose == OpenCloseType::OpenCloseType_Open)
	{
		pField->Position += pTrade->Qty;
		pField->TodayPosition += pTrade->Qty;
	}
	else
	{
		pField->Position -= pTrade->Qty;
		if (pTrade->OpenClose == OpenCloseType::OpenCloseType_CloseToday)
		{
			pField->TodayPosition -= pTrade->Qty;
		}
		else
		{
			pField->HistoryPosition -= pTrade->Qty;
			// �������ı����ɸ������ӽ��쿪ʼ������
			if (pField->HistoryPosition < 0)
			{
				pField->TodayPosition += pField->HistoryPosition;
				pField->HistoryPosition = 0;
			}
		}

		// �������ֱ�����²�ѯ
		if (pField->Position < 0 || pField->TodayPosition < 0 || pField->HistoryPosition < 0)
		{
			//ReqQryInvestorPosition("", "");
			ReqQueryField body = { 0 };
			ReqQuery(QueryType::QueryType_ReqQryInvestorPosition, &body);
			return;
		}
	}

	m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRspQryInvestorPosition, m_msgQueue, m_pClass, false, 0, pField, sizeof(PositionField), nullptr, 0, nullptr, 0);
}

void CTraderApi::OnOrderEvent(XTPOrderInfo *order_info, XTPRI *error_info, uint64_t session_id)
{
	OrderIDType orderId = { 0 };
	sprintf(orderId, "%ld:%lu", order_info->insert_time, order_info->order_xtp_id);
	OrderIDType orderSydId = { 0 };

	{
		// ����ԭʼ������Ϣ�����ڳ���

		unordered_map<string, XTPOrderInfo*>::iterator it = m_id_api_order.find(orderId);
		if (it == m_id_api_order.end())
		{
			// �Ҳ����˶�������ʾ���µ�
			XTPOrderInfo* pField = (XTPOrderInfo*)m_msgQueue->new_block(sizeof(XTPOrderInfo));
			memcpy(pField, order_info, sizeof(XTPOrderInfo));
			m_id_api_order.insert(pair<string, XTPOrderInfo*>(orderId, pField));
		}
		else
		{
			// �ҵ��˶���
			// ��Ҫ�ٸ��Ʊ������һ�ε�״̬������ֻҪ��һ�ε����ڳ������ɣ����£��������ñȽ�
			XTPOrderInfo* pField = it->second;
			memcpy(pField, order_info, sizeof(XTPOrderInfo));
		}

		// ����SysID���ڶ���ɽ��ر��붩��
		sprintf(orderSydId, "%d:%s:%lu", (int)order_info->market, order_info->ticker, order_info->order_xtp_id);
		m_sysId_orderId.insert(pair<string, string>(orderSydId, orderId));
	}

	{
		// ��API�Ķ���ת�����Լ��Ľṹ��
		OrderField* pOut = (OrderField*)m_msgQueue->new_block(sizeof(OrderField));

		strcpy(pOut->ID, orderId);
		strcpy(pOut->LocalID, pOut->ID);
		sprintf(pOut->OrderID, "%lu", order_info->order_xtp_id);
		strcpy(pOut->ExchangeID, XTPMarketType_2_Str(order_info->market));
		strcpy(pOut->Symbol, order_info->ticker);
		pOut->Price = order_info->price;
		pOut->Qty = order_info->quantity;
		pOut->Side = (OrderSide)OrderDir_XTP_2_XAPI(order_info->side);
		pOut->OpenClose = (OpenCloseType)OpenClose_XTP_2_XAPI(order_info->position_effect);
		pOut->Status = (OrderStatus)OrderStatus_XTP_2_XAPI(order_info->order_status);
		pOut->Type = (OrderType)OrderType_XTP_2_XAPI(order_info->order_type);
		pOut->CumQty = order_info->qty_traded;
		pOut->LeavesQty = order_info->qty_left;

		m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRspQryOrder, m_msgQueue, m_pClass, true, 0, pOut, sizeof(OrderField), nullptr, 0, nullptr, 0);

		m_msgQueue->delete_block(pOut);
	}
}
void CTraderApi::OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id)
{
	TradeField* pOut = (TradeField*)m_msgQueue->new_block(sizeof(TradeField));

	strcpy(pOut->ExchangeID, XTPMarketType_2_Str(trade_info->market));
	strcpy(pOut->Symbol, trade_info->ticker);
	pOut->Price = trade_info->price;
	pOut->Qty = trade_info->quantity;
	pOut->Side = (OrderSide)OrderDir_XTP_2_XAPI(trade_info->side);
	pOut->OpenClose = (OpenCloseType)OpenClose_XTP_2_XAPI(trade_info->position_effect);
	strcpy(pOut->TradeID, trade_info->exec_id);


	OrderIDType orderSysId = { 0 };
	sprintf(orderSysId, "%d:%s:%lu", (int)trade_info->market, trade_info->ticker, trade_info->order_xtp_id);
	unordered_map<string, string>::iterator it = m_sysId_orderId.find(orderSysId);
	if (it == m_sysId_orderId.end())
	{
		// �˳ɽ��Ҳ�����Ӧ�ı���
		//assert(false);
	}
	else
	{
		strcpy(pOut->ID, it->second.c_str());
		m_msgQueue->Input_Copy(ResponseType::ResponseType_OnRtnTrade, m_msgQueue, m_pClass, true, 0, pOut->ID, sizeof(TradeField), nullptr, 0, nullptr, 0);
	}

	m_msgQueue->delete_block(pOut);
}
void CTraderApi::OnQueryOrder(XTPQueryOrderRsp *order_info, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id) {}
void CTraderApi::OnQueryTrade(XTPQueryTradeRsp *trade_info, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id) {}
void CTraderApi::OnQueryPosition(XTPQueryStkPositionRsp *position, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id) {}
void CTraderApi::OnQueryAsset(XTPQueryAssetRsp *asset, XTPRI *error_info, int request_id, bool is_last, uint64_t session_id) {}

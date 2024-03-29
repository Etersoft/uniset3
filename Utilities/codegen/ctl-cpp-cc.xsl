<?xml version='1.0' encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'
                     xmlns:date="http://exslt.org/dates-and-times">

<xsl:import href="ctl-cpp-common.xsl"/>
<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:variable name="CLASSNAME">
    <xsl:call-template name="settings"><xsl:with-param name="varname" select="'class-name'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="BASECLASS">
    <xsl:call-template name="settings"><xsl:with-param name="varname" select="'base-class'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="OID">
    <xsl:call-template name="settings"><xsl:with-param name="varname" select="'ID'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="TESTMODE">
    <xsl:call-template name="settings"><xsl:with-param name="varname" select="'testmode'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="ARGPREFIX">
    <xsl:call-template name="settings"><xsl:with-param name="varname" select="'arg-prefix'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="LOGROTATE">
    <xsl:call-template name="settings"><xsl:with-param name="varname" select="'logrotate'"/></xsl:call-template>
</xsl:variable>
<!-- Генерирование cc-файла -->
<xsl:template match="/">

<!-- BEGIN CC-FILE -->
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-HEAD"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-FILE"/>
// --------------------------------------------------------------------------
<xsl:call-template name="COMMON-CC-FUNCS"/>
// --------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::callback() noexept
{
    if( !active )
        return;
    try
    {    
        // NOTE: Нужно ли здесь опрашивать текущее значение выходов?!!
        // Опрос входов
        updateValues();
<xsl:if test="normalize-space($TESTMODE)!=''">
        if( idLocalTestMode_S != DefaultObjectId )
        {
            isTestMode = checkTestMode();
            if( trTestMode.change(isTestMode) )
                testMode(isTestMode);

            if( isTestMode )
            {
                if( !active )
                    return;

                msleep( sleep_msec );
                return;
            }
        }
</xsl:if>
        checkSensors();

        // проверка таймеров
        checkTimers(this);

        if( resetMsgTime&gt;0 &amp;&amp; trResetMsg.hi(ptResetMsg.checkTime()) )
        {
//            cout &lt;&lt; myname &lt;&lt;  ": ********* reset umessage *********" &lt;&lt; endl;
            resetMsg();
        }

        // обработка сообщений (таймеров и т.п.)
        for( unsigned int i=0; i&lt;<xsl:call-template name="settings"><xsl:with-param name="varname" select="'msg-count'"/></xsl:call-template>; i++ )
        {
            auto m = receiveMessage();
            if( !m )
                break;
            processingMessage(m.get());
        }

        // Выполнение шага программы
        step();

        // "сердцебиение"
        if( idHeartBeat!=DefaultObjectId &amp;&amp; ptHeartBeat.checkTime() )
        {
            try
            {
                ui->setValue(idHeartBeat,maxHeartBeat);
                ptHeartBeat.reset();
            }
            catch( const uniset3::Exception&amp; ex )
            {
                mycrit &lt;&lt; myname &lt;&lt; "(execute): " &lt;&lt; ex &lt;&lt; endl;
            }
        }

        // Формирование выходов
        updateOutputs(forceOut);
        updatePreviousValues();
    }
    catch( const std::exception&amp;ex )
    {
        mycrit &lt;&lt; myname &lt;&lt; "(execute): catch " &lt;&lt; ex.what()  &lt;&lt;   endl;
    }

    if( !active )
        return;
    
    msleep( sleep_msec );
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::setValue( uniset3::ObjectId sid, long val )
{
    if( _sid == uniset3::DefaultObjectId )
        return;
        
    <xsl:for-each select="//smap/item">
    <xsl:if test="normalize-space(@vartype)='out'">
    if( sid == <xsl:value-of select="@name"/> )
    {
        mylog8 &lt;&lt;  "(setValue): <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = " &lt;&lt;  val &lt;&lt;  endl;
        <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>    = val;
        return;
    }
    </xsl:if>
    </xsl:for-each>

    ui->setValue(sid,val);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::updateOutputs( bool force )
{
<xsl:for-each select="//smap/item">
<xsl:choose>
    <xsl:when test="normalize-space(@vartype)='out'"><xsl:call-template name="setdata"/></xsl:when>
</xsl:choose>
</xsl:for-each>

<!--
// update umessage
<xsl:for-each select="//msgmap/item">
    si.id = <xsl:value-of select="@name"/>;
    ui->setValue( si,m_<xsl:value-of select="@name"/>,getId() );
</xsl:for-each>
-->
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::preAskSensors( uniset3::UIOCommand cmd )
{
    <xsl:for-each select="//smap/item">
        <xsl:choose>
            <xsl:when test="normalize-space(@vartype)='in'"><xsl:call-template name="check_changes"><xsl:with-param name="onlymsg" select="1"/></xsl:call-template></xsl:when>
        </xsl:choose>            
    </xsl:for-each>
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::askSensor( uniset3::ObjectId sid, uniset3::UIOCommand cmd, uniset3::ObjectId node )
{
    if( cmd == uniset3::UIONotify )
    {
        SensorMessage _sm = makeSensorMessage(sid, ui->getValue(sid,node), uniset3::AI);
        _sm.mutable_header()->set_consumer(getId());
        _sm.mutable_header()->set_node(node);
        sensorInfo(&amp;sm);
    }
}
// -----------------------------------------------------------------------------
long <xsl:value-of select="$CLASSNAME"/>_SK::getValue( uniset3::ObjectId _sid )
{
    <xsl:for-each select="//smap/item">
    if( _sid == <xsl:value-of select="@name"/> )
        return <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>;
    </xsl:for-each>

    return ui->getValue(_sid);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::preSensorInfo( const uniset3::umessage::SensorMessage* sm )
{
    sensorInfo(sm);
}
// -----------------------------------------------------------------------------
void <xsl:value-of select="$CLASSNAME"/>_SK::initFromSM()
{
    <xsl:for-each select="//smap/item">
    <xsl:if test="normalize-space(@initFromSM)!=''">
    if( <xsl:value-of select="@name"/> != uniset3::DefaultObjectId )
    {
        try
        {
            <xsl:if test="normalize-space(@vartype)='in'">priv_</xsl:if><xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui->getValue(<xsl:value-of select="@name"/>,node_<xsl:value-of select="@name"/>);
        }
        catch( std::exception&amp; ex )
        {
            mycrit &lt;&lt; myname &lt;&lt; "(initFromSM): " &lt;&lt; ex.what() &lt;&lt; endl;
        }
    }
    </xsl:if>
    </xsl:for-each>
}
<!-- END CC-FILE -->
</xsl:template>

<xsl:template name="getdata">
<xsl:param name="output">0</xsl:param>    
    try
    {
        if( <xsl:value-of select="@name"/> != DefaultObjectId )
            priv_<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> = ui->getValue(<xsl:value-of select="@name"/>,node_<xsl:value-of select="@name"/>);
    }
    catch( const uniset3::Exception&amp; ex )
    {
        mycrit &lt;&lt; myname &lt;&lt; "(getdata): " &lt;&lt; ex &lt;&lt; endl;
        throw;
    }
</xsl:template>

<xsl:template name="setdata">
    try
    {
        if( <xsl:value-of select="@name"/> != DefaultObjectId ) // -V547
        {
            si.set_id(<xsl:value-of select="@name"/>);
            si.set_node(node_<xsl:value-of select="@name"/>);
            ui->setValue( si, <xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>, getId() );
        }
    }
    catch( const uniset3::Exception&amp; ex )
    {
        mycrit &lt;&lt; myname &lt;&lt; "(setdata): " &lt;&lt; ex &lt;&lt; endl;
        throw;
    }
</xsl:template>

<xsl:template name="setdata_value">
<xsl:param name="setval">0</xsl:param>    
    try
    {
        if( <xsl:value-of select="@name"/> != DefaultObjectId )
        {
            si.set_id(<xsl:value-of select="@name"/>);
            si.set_node(node_<xsl:value-of select="@name"/>);
            ui->setValue( si,<xsl:value-of select="$setval"/>, getId() );
        }
    }
    catch( const uniset3::Exception&amp; ex )
    {
        mycrit &lt;&lt; myname &lt;&lt; "(setdata): " &lt;&lt; ex &lt;&lt; endl;
        throw;
    }
</xsl:template>

<xsl:template name="setmsg_val">
    try
    {
        if( <xsl:value-of select="@name"/> != DefaultObjectId )
        {
            si.set_id(<xsl:value-of select="@name"/>);
            si.set_node(node_<xsl:value-of select="@name"/>);
            ui->setValue( si,(long)m_<xsl:value-of select="@name"/>, getId() );
        }
    }
    catch( const uniset3::Exception&amp; ex )
    {
        mycrit &lt;&lt; myname &lt;&lt; "(setmsg): " &lt;&lt; ex &lt;&lt; endl;
        throw;
    }
</xsl:template>

</xsl:stylesheet>

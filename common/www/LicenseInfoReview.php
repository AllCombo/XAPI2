<?php
session_start();
require_once('config.php');
include_once('CheckSignin.php');
require_once('EnumFun.php');

$result = $mdb2->query(
	'SELECT * FROM LicenceInfo WHERE '
    ." ID = "
    .$mdb2->quote($_GET['ID'],"integer")
    );
if (PEAR::isError($result)) {
	die("{'Error':'".$result->getMessage()."'}");
}
$row = $result->fetchRow();

$product = $row[strtolower('Product')];

$content1 = strval($row[strtolower('Content1')]);
$content2 = strval($row[strtolower('Content2')]);

$json_content1 = json_decode($content1,true);
$json_content2 = json_decode($content2,true);
?>
<!DOCTYPE HTML><html>
<head>
<meta charset="UTF-8">
<title>授权审核</title>
</head>
<body>
<?php 
if(in_array($product,$_SESSION['Products'],true))
{
}
else
{
    if($_SESSION['Right'] > 2)
    {
        // 管理员是有权限审核的
    }
    else
    {
        echo "您没有权限审核此授权信息";
        return;       
    }
}
?>
<table border="1">
  <tr>
  <th>ID</th>
  <td><?php echo $row[strtolower('ID')];?></td>
  <th>产品</th>
  <td><?php echo $row[strtolower('Product')];?></td>
  </tr>
  <tr>
  <th>申请人员</th>
  <td><?php echo $row[strtolower('User1')];?></td>
  <th>审核人员</th>
  <td><?php echo $row[strtolower('User2')];?></td>
  </tr>
  <tr>
  <th>申请时间</th>
  <td><?php echo $row[strtolower('Time1')];?></td>
  <th>审核时间</th>
  <td><?php echo $row[strtolower('Time2')];?></td>
  </tr>
  <tr>
  <th>申请内容</th>
  <td><pre><?php print_r(json_decode($content1,true));?></pre></td>
  <th>审核内容</th>
  <td><pre><?php print_r(json_decode($content2,true));?></pre></td>
  </tr>
  <tr>
  <th>状态</th>
  <td><?php echo EnumStatusToString($row[strtolower('Status')]);?></td>
  <th>备注</th>
  <td><?php echo $row[strtolower('Remark')];?></td>
  </tr>
</table>
<hr/>
<?php if($row[strtolower('Status')]<4) {?>
<table>
  <tr>
  <td>

  <form action="LicenseInfoAction.php?ID=<?php echo $row[strtolower('ID')];?>" method="post">
<p>这个表格中自动填写的是申请内容</p>
<p>Product: <input type="text" name="Product" readonly value="<?php echo $row[strtolower('Product')];?>"/></p>
<p>ExpireDate: <input type="text" name="ExpireDate" value="<?php echo $json_content1['License']['ExpireDate'];?>"/>到期时间，格式：20160315</p>
<p>Trial: <input type="text" name="Trial" value="<?php echo $json_content1['License']['Trial'];?>"/>单次会话下单次数，0表示无限制</p>
<p>MachineID: <input type="text" name="MachineID" value="<?php echo $json_content1['License']['MachineID'];?>"/>机器码，如果没有绑定账号就必须绑定机器码，支持正则</p>
<p>Account: <input type="text" name="Account" value="<?php echo $json_content1['User']['Account'];?>"/>登录时所用的客户号，支持正则</p>
<p>UserName: <input type="text" name="UserName" value="<?php echo $json_content1['User']['UserName'];?>"/>接口登录后所能取到的客户真实姓名，不支持正则</p>
<p>Info: <input type="text" name="Info" value="<?php echo $json_content1['User']['Info'];?>"/>额外信息，一般由审核员填写，会写入到授权文件中</p>
<hr/>
<p>状态: <select name="Status">
  <option value ="1"><?php echo EnumStatusToString(1);?></option>
  <option value ="2"><?php echo EnumStatusToString(2);?></option>
  <option value ="3"><?php echo EnumStatusToString(3);?></option>
  <option value ="4"><?php echo EnumStatusToString(4);?></option>
</select>状态信息决定了是否审核完成，用户是否可下载，不要改错了</p>
<p>备注: <textarea rows="5" cols="60" name="Remark"><?php echo $row[strtolower('Remark')];?></textarea>备注，需告知审核员或用户的消息</p>
<input type="submit" value="Submit" name="Review_Left" />
</form>

</td>
<td>

<form action="LicenseInfoAction.php?ID=<?php echo $row[strtolower('ID')];?>" method="post">
<p>这个表格中自动填写的是审核内容</p>
<p>Product: <input type="text" name="Product" readonly value="<?php echo $row[strtolower('Product')];?>"/></p>
<p>ExpireDate: <input type="text" name="ExpireDate" value="<?php echo $json_content2['License']['ExpireDate'];?>"/></p>
<p>Trial: <input type="text" name="Trial" value="<?php echo $json_content2['License']['Trial'];?>"/></p>
<p>MachineID: <input type="text" name="MachineID" value="<?php echo $json_content2['License']['MachineID'];?>"/></p>
<p>Account: <input type="text" name="Account" value="<?php echo $json_content2['User']['Account'];?>"/></p>
<p>UserName: <input type="text" name="UserName" value="<?php echo $json_content2['User']['UserName'];?>"/></p>
<p>Info: <input type="text" name="Info" value="<?php echo $json_content2['User']['Info'];?>"/></p>
<hr/>
<p>状态: <select name="Status">
  <option value ="1"><?php echo EnumStatusToString(1);?></option>
  <option value ="2"><?php echo EnumStatusToString(2);?></option>
  <option value ="3"><?php echo EnumStatusToString(3);?></option>
  <option value ="4"><?php echo EnumStatusToString(4);?></option>
</select></p>
<p>备注: <textarea rows="5" cols="60" name="Remark"><?php echo $row[strtolower('Remark')];?></textarea></p>
<input type="submit" value="Submit"  name="Review_Right" />
</form>

</td>
</tr>
</table>
<?php }?>

</body>
</html>
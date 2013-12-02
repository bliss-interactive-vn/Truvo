<?php

function _vat_get_week_text($t_week, $t_year) {
	$first_date_week = strtotime($t_year . 'W' . str_pad($t_week, 2, '0', STR_PAD_LEFT));
	$last_date_week = strtotime('+6 days', $first_date_week);

	$s_first_date_week = date('d/m', $first_date_week);
	$s_last_date_week = date('d/m', $last_date_week);

	return "Week $t_week of $t_year ($s_first_date_week - $s_last_date_week)";
}

function _vat_generate_week_list($f_week_year_s = '') {
	$c_year =  date('Y');
	$c_week =  date('W');

	$c_week_year = $c_year . $c_week;

	$next_week_t = strtotime('+21 days'); // next 3 weeks
	$n_year =  date('Y', $next_week_t);
	$n_week =  date('W', $next_week_t);

	$n_week_year = $n_year . $n_week;

	if (empty($f_week_year_s)) {
		$f_week_year_s = '44-2013';
	}

	list($t_week, $t_year) = explode('-', $f_week_year_s);
	$t_week_year = $t_year . $t_week;


	$result = array();
	while ($t_week_year <= $n_week_year) {
		$t_week_year_s = "$t_week-$t_year";		
		$t_week_year_s_detail =_vat_get_week_text($t_week, $t_year);
		$result[$t_week_year_s] = $t_week_year_s_detail;

		$first_date_week = strtotime($t_year . 'W' . str_pad($t_week, 2, '0', STR_PAD_LEFT));

		if ($t_week >= 52) {
			$t_year++;
			$t_week = '01';
		}
		else {
			$next_t_week_t = strtotime('+7 days', $first_date_week);
			$t_year =  date('Y', $next_t_week_t);
			$t_week =  date('W', $next_t_week_t);
			$t_week_year = $t_year . $t_week;
		}
	}
	return $result;
}

// RADIO ------------------------------------

// radio list
function vat_admin_radio_list($lang = '-', $week = '-', $keywords = '') {

	$output = drupal_render(drupal_get_form('vat_admin_radio_filter_form', $lang, $week, $keywords));

	$header = array(
		array('data' => 'ID', 'field' => 'id', 'sort' => 'asc'),
		array('data' => 'File', 'field' => 'file', 'sort' => 'asc'),
		array('data' => 'Language', 'field' => 'lang', 'sort' => 'asc'),
		array('data' => 'Week', 'field' => 'week', 'sort' => 'asc'),
		array('data' => 'Is active ?', 'field' => 'is_active', 'sort' => 'asc'),
		array('data' => 'Created on', 'field' => 'created', 'sort' => 'asc'),
		array('data' => 'Operators')
	);

	$select = db_select('vat_radios', 'r')
		->fields('r')
		// ->condition('r.version', '3')
		->extend('TableSort')
		->extend('PagerDefault')->limit(20);

	if ($lang != '-') {		
		$select->condition('r.lang', $lang);
	}

	if ($week != '-') {	
		$select->condition('r.week', $week);
	}

	if ($keywords) {
		$or = db_or()
		->condition('r.file', '%' . $keywords . '%', 'LIKE');
		
		$select->condition($or);
	}

	$results = $select->orderByHeader($header)->execute();	

	$rows = array();
	foreach ($results as $data) {
		list($t_week, $t_year) = explode('-', $data->week);
		$data_week = _vat_get_week_text($t_week, $t_year);
		$row = array(
			check_plain($data->id), 
			l(basename($data->file), $data->file, array("attributes" => array("target" => "_blank"))), 
			check_plain(strtoupper($data->lang)), 
			check_plain($data_week),
			$data->is_active ? '<span style="color:green">' . t('Yes') . '</span>' : '<span style="color:red">' . t('No') . '</span>', 
			check_plain($data->created),
			l(t($data->is_active ? 'Deactivate' : 'Activate'), 'admin/config/vat/radio/' . $data->id . '/' . ($data->is_active ? 'deactivate' : 'activate'))
			. ' | ' . 
			l(t('Delete'), 'admin/config/vat/radio/' . $data->id . '/delete')
		);
		$rows[] = $row;
	}
	
	$output .= theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No radio file found.')));
	$output .= theme('pager', array('tags' => array()));
	
	return $output;
}
function vat_admin_radio_filter_form($form, &$form_state, $lang = '-', $week = '-', $keywords = '') {
	$form['#attributes'] = array('class' => 'search-form');
	
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Filter by')
	);
	
	$form['basic']['inline'] = array('#prefix' => '<div class="container-inline">', '#suffix' => '</div>');
	
	$form['basic']['inline']['filter'] = array(
		'#type' => 'textfield',
		'#title' => 'Keyword:',
		'#default_value' => $keywords,
		'#maxlength' => 128,
		'#size' => 25,
	);

	$week_list = _vat_generate_week_list();
	$form['basic']['inline']['week'] = array(
		'#type' => 'select',
		'#title' => t('Week:'),
		'#options' => array('-' => t('All')) + $week_list,
		'#default_value' => $week,
	);

	$form['basic']['inline']['lang'] = array(
		'#type' => 'select',
		'#title' => t('Language:'),
		'#options' => array(
			'-' => t('All'),
			'NL' => t('NL'),
			'FR' => t('FR'),
		),
		'#default_value' => $lang,
	);		
	
	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_radio_filter_form_submit'),
	);

	if (!empty($keywords) || $lang != '-') {
		$form['basic']['inline']['reset'] = array(
			'#type' => 'submit',
			'#value' => t('Reset'),
			'#submit' => array('vat_admin_radio_filter_form_reset'),
		);
	}
	return $form;
}
function vat_admin_radio_filter_form_submit($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/radio/list/' . trim($form_state['values']['lang']) . '/' . trim($form_state['values']['week']) . '/' . trim($form_state['values']['filter']);
}
function vat_admin_radio_filter_form_reset($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/radio/list';
}

function vat_admin_radio_delete_confirm($form, &$form_state, $id) {
  $form['delete'] = array(
    '#type' => 'value',
    '#value' => $id,
  );
  return confirm_form(
    $form,
    t('Are you sure you want to delete this?'),
    'admin/config/vat/radio',
    '',
    t('Delete'),
    t('Cancel')
  );
}
function vat_admin_radio_delete_confirm_submit($form, &$form_state) { 
  $record = $form_state['values']['delete'];
  if ($record) {
    $num_deleted = db_delete('vat_radios')
      ->condition('id', $record)
      ->execute();
      
    drupal_set_message('The record has been deleted!');
  }
  $form_state['redirect'] = "admin/config/vat/radio";
}

function vat_admin_radio_activate($id) {
  $num_updated = db_update('vat_radios')
  ->fields(array(
    'is_active' => 1
  ))
  ->condition('id', $id)
  ->execute();

  drupal_set_message('Your record has been set active!');
  drupal_goto("admin/config/vat/radio");
}

function vat_admin_radio_deactivate($id) {
  $num_updated = db_update('vat_radios')
  ->fields(array(
    'is_active' => 0
  ))
  ->condition('id', $id)
  ->execute();

  drupal_set_message('Your record has been set inactive!');
  drupal_goto("admin/config/vat/radio");
}

// new radio
function vat_admin_radio_new() {
	drupal_set_title(check_plain('Add new radio file'));

	$upoad_folder = drupal_get_path('module', 'vat') . '/uploads/radios/';
	if (!is_writable($upoad_folder))
		drupal_set_message('Warning: Folder "' . $upoad_folder . '" is not writable. Our file will not be able to store there.', 'warning');

	$output = drupal_render(drupal_get_form('vat_admin_radio_new_form'));
	return $output;
}
function vat_admin_radio_new_form($form, &$form_state) {
	$form['#attributes'] = array('class' => 'search-form');
	
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Add new radio file')
	);
	
	$form['basic']['inline'] = array('#prefix' => '<div>', '#suffix' => '</div>');
	
	$form['basic']['inline']['file'] = array(
		'#type' => 'file',
		'#title' => t('Select an audio file:'),
	);
	
	$form['basic']['inline']['lang'] = array(
		'#type' => 'select',
		'#title' => t('Language:'),
		'#options' => array(
			'NL' => t('NL'),
			'FR' => t('FR'),
		),
	);

	$week_list = _vat_generate_week_list();
	$form['basic']['inline']['week'] = array(
		'#type' => 'select',
		'#title' => t('Select a week:'),
		'#options' => $week_list,
		'#default_value' => date('W-Y'),
	);

	$form['basic']['inline']['is_active'] = array(
		'#type' => 'checkbox',
		'#title' => t('Is active ?'),
		'#value' => 1,
	);

	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_radio_new_form_submit'),
	);
		
	$form['basic']['inline']['back'] = array(
		'#type' => 'submit',
		'#value' => t('Back'),
		'#submit' => array('vat_admin_radio_new_form_back'),
	);
	return $form;
}
function vat_admin_radio_new_form_submit($form, &$form_state) {
	if ($form_state['submitted']) {
		$allowedExts = array('wav', 'mp3', 'ogg', 'wma', 'flac');
		$temp = explode('.', $_FILES['files']['name']['file']);
		$extension = end($temp);
		if (in_array($extension, $allowedExts)) {
			if ($_FILES['files']['error']['file'] > 0) {
				drupal_set_message("Error: " . $_FILES['files']['error']['file'], 'error');
			}
			else {
				$lang = $form_state['input']['lang'];
				$week = $form_state['input']['week'];
				$is_active = isset($form_state['input']['is_active']) ? $form_state['input']['is_active'] : 0;

				$file = drupal_get_path('module', 'vat') . '/uploads/radios/' . rand() . '_' . preg_replace('/[^a-zA-Z0-9\.]/', '', $_FILES['files']['name']['file']);
				
				if (move_uploaded_file($_FILES['files']['tmp_name']['file'], $file)) {
					db_insert('vat_radios')
					->fields(array(
							'file' => $file,
							'lang' => $lang,
							'week' => $week,
							'is_active' => $is_active,
							'created' => date('Y-m-d H:i:s')
					))
					->execute();

					$radio_id = Database::getConnection()->lastInsertId();
					if (!empty($radio_id)) {
						drupal_set_message(t("Your file has been recorded successfully!"));
					}
					else {
						drupal_set_message(t("Error: Your file has not been recorded!"), 'error');				
					}

				}
				else {
					drupal_set_message(t("Error: Upload failed!"), 'error');
				}
			}
		}
		else {
			drupal_set_message(t("Error: Invalid file"), 'error');
		}
	}
	
  return TRUE;
}
function vat_admin_radio_new_form_back($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/radio';
}




// VAT NUMBER ------------------------------------

// btw_number list
function vat_admin_btw_number_list($status = '1', $lang = '-', $week = '-', $keywords = '') {

	$output = drupal_render(drupal_get_form('vat_admin_btw_number_filter_form', $status, $lang, $week, $keywords));

	if ($status == '0') {
		$output .= drupal_render(drupal_get_form('vat_admin_btw_number_notactiveyet_form'));
	}

	$header = array(
		array('data' => 'ID', 'field' => 'id', 'sort' => 'asc'),
		array('data' => 'VAT Number', 'field' => 'number', 'sort' => 'asc'),
		array('data' => 'Language', 'field' => 'lang', 'sort' => 'asc'),
		array('data' => 'Week', 'field' => 'week', 'sort' => 'asc'),
		array('data' => 'Created on', 'field' => 'created', 'sort' => 'asc'),
		// array('data' => 'Operators')
	);

	$select = db_select('vat_btw_numbers', 'n')
		->fields('n')
		->condition('n.is_active', $status)
		->extend('TableSort')
		->extend('PagerDefault')->limit(100);

	if ($lang != '-') {	
		$select->condition('n.lang', $lang);
	}

	if ($week != '-') {	
		$select->condition('n.week', $week);
	}

	if ($keywords) {
		$or = db_or()
		->condition('n.number', '%' . $keywords . '%', 'LIKE');
		
		$select->condition($or);
	}

	$results = $select->orderByHeader($header)->execute();	

	$rows = array();
	foreach ($results as $data) {
		list($t_week, $t_year) = explode('-', $data->week);
		$data_week = _vat_get_week_text($t_week, $t_year);
		$row = array(
			check_plain($data->id), 
			check_plain($data->number), 
			check_plain(strtoupper($data->lang)), 
			check_plain($data_week), 
			check_plain($data->created),
			// l(t('Delete'), 'admin/config/vat/radio/' . $data->id . '/delete')
		);
		$rows[] = $row;
	}
	
	$output .= theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No VAT number found.')));
	$output .= theme('pager', array('tags' => array()));
	
	return $output;
}
function vat_admin_btw_number_filter_form($form, &$form_state, $status = '1', $lang = '-', $week = '-', $keywords = '') {
	$form['#attributes'] = array('class' => 'search-form');
	
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Filter by')
	);
	
	$form['basic']['inline'] = array('#prefix' => '<div class="container-inline">', '#suffix' => '</div>');
	
	$form['basic']['inline']['filter'] = array(
		'#type' => 'textfield',
		'#title' => 'Keyword:',
		'#default_value' => $keywords,
		'#maxlength' => 128,
		'#size' => 25,
	);

	$week_list = _vat_generate_week_list();
	$form['basic']['inline']['week'] = array(
		'#type' => 'select',
		'#title' => t('Week:'),
		'#options' => array('-' => t('All')) + $week_list,
		'#default_value' => $week,
	);
	
	$form['basic']['inline']['lang'] = array(
		'#type' => 'select',
		'#title' => t('Language:'),
		'#options' => array(
			'-' => t('All'),
			'NL' => t('NL'),
			'FR' => t('FR'),
		),
		'#default_value' => $lang,
	);
	
	$form['basic']['inline']['status'] = array(
		'#type' => 'select',
		'#title' => t('Status:'),
		'#options' => array(
			'1' => t('Active'),
			'0' => t('Not active yet'),
		),
		'#default_value' => $status,
	);

	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_btw_number_filter_form_submit'),
		);
		
	if (!empty($keywords) || $week != '-' || $lang != '-' || $status != '1') {
		$form['basic']['inline']['reset'] = array(
			'#type' => 'submit',
			'#value' => t('Reset'),
			'#submit' => array('vat_admin_btw_number_filter_form_reset'),
		);
	}
	return $form;
}
function vat_admin_btw_number_filter_form_submit($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/btw_number/list/' . trim($form_state['values']['status']) . '/' . trim($form_state['values']['lang']) . '/' . trim($form_state['values']['week']) . '/' . trim($form_state['values']['filter']);
}
function vat_admin_btw_number_filter_form_reset($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/btw_number/list';
}
function vat_admin_btw_number_notactiveyet_form($form, &$form_state) {
	$form['#attributes'] = array('class' => 'search-form');
	
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('What you can do with these not-active-yet VAT numbers:')
	);
	
	$form['basic']['inline'] = array('#prefix' => '<div class="container-inline">', '#suffix' => '</div>');
	
	$form['basic']['inline']['make_active'] = array(
		'#type' => 'submit',
		'#value' => t('Make them all active'),
		'#submit' => array('vat_admin_btw_number_notactiveyet_form_make_active'),
	);

	$form['basic']['inline']['delete'] = array(
		'#type' => 'submit',
		'#value' => t('Delete all'),
		'#submit' => array('vat_admin_btw_number_notactiveyet_form_delete'),
	);	

	return $form;
}
function vat_admin_btw_number_notactiveyet_form_make_active($form, &$form_state) {
	if ($form_state['submitted']) {
		$num_updated = db_update('vat_btw_numbers')
	  ->fields(array(
	    'is_active' => 1
	  ))
	  ->condition('is_active', 0)
	  ->execute();
		
		if ($num_updated >  0)
			drupal_set_message($num_updated . ' records have been set active successfully!');
		else
			drupal_set_message('No record has been set active', 'warning');
	}

	$form_state['redirect'] = 'admin/config/vat/btw_number/list';
}
function vat_admin_btw_number_notactiveyet_form_delete($form, &$form_state) {
	if ($form_state['submitted']) {
		$num_deleted = db_delete('vat_btw_numbers')
	  ->condition('is_active', 0)
	  ->execute();
		
		if ($num_deleted > 0)
			drupal_set_message($num_deleted . ' records have been deleted successfully!');	
		else
			drupal_set_message('No record has been deleted', 'warning');
	}

	$form_state['redirect'] = 'admin/config/vat/btw_number/list';
}

function vat_admin_btw_number_delete_confirm($form, &$form_state, $id) {
  $form['delete'] = array(
    '#type' => 'value',
    '#value' => $id,
  );
  return confirm_form(
    $form,
    t('Are you sure you want to delete this?'),
    'admin/config/vat/btw_number',
    '',
    t('Delete'),
    t('Cancel')
  );
}
function vat_admin_btw_number_delete_confirm_submit($form, &$form_state) {
  $record = $form_state['values']['delete'];
  if ($record) {
    $num_deleted = db_delete('vat_btw_numbers')
      ->condition('id', $record)
      ->execute();
      
    drupal_set_message('The record has been deleted!');
  }
  $form_state['redirect'] = "admin/config/vat/btw_number";
}

// import btw_number
function vat_admin_btw_number_import() {
	drupal_set_title(check_plain('Import VAT Number from CSV file'));

	$upoad_folder = drupal_get_path('module', 'vat') . '/uploads/btw_numbers/';
	if (!is_writable($upoad_folder))
		drupal_set_message('Warning: Folder "' . $upoad_folder . '" is not writable. Our file will not be able to store there.', 'warning');

	$output = drupal_render(drupal_get_form('vat_admin_btw_number_import_form'));
	return $output;
}
function vat_admin_btw_number_import_form($form, &$form_state) {
	$form['#attributes'] = array('class' => 'search-form');
	
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Import VAT Number from CSV file')
	);
	
	$form['basic']['inline'] = array('#prefix' => '<div>', '#suffix' => '</div>');
	
	$form['basic']['inline']['file'] = array(
		'#type' => 'file',
		'#title' => t('Select a CSV file:') . ' - ' . l('example.csv', drupal_get_path('module', 'vat') . '/uploads/btw_numbers/example.csv', array("attributes" => array("target" => "_blank"))),
	);

	$week_list = _vat_generate_week_list();
	$form['basic']['inline']['week'] = array(
		'#type' => 'select',
		'#title' => t('Select a week for these VAT numbers:'),
		'#options' => $week_list,
		'#default_value' => date('W-Y'),
	);

	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_btw_number_import_form_submit'),
	);
		
	$form['basic']['inline']['back'] = array(
		'#type' => 'submit',
		'#value' => t('Back'),
		'#submit' => array('vat_admin_btw_number_import_form_back'),
	);
	return $form;
}
function vat_admin_btw_number_import_form_submit($form, &$form_state) {
	if ($form_state['submitted']) {
		$allowedExts = array('csv');
		$temp = explode('.', $_FILES['files']['name']['file']);
		$extension = end($temp);
		if (in_array($extension, $allowedExts)) {
			if ($_FILES['files']['error']['file'] > 0) {
				drupal_set_message("Error: " . $_FILES['files']['error']['file'], 'error');
			}
			else {
				$file = drupal_get_path('module', 'vat') . '/uploads/btw_numbers/' . rand() . '_' . preg_replace('/[^a-zA-Z0-9\.]/', '', $_FILES['files']['name']['file']);
				
				if (move_uploaded_file($_FILES['files']['tmp_name']['file'], $file)) {

					$week = $form_state['input']['week'];		

					$hfile = fopen($file,"r");
					$i = 0;
					while(!feof($hfile)) {
				  	$row = fgetcsv($hfile);
				  	if (count($row) == 2 && is_numeric($row[0])) 
					{
						$number = $row[0];
						if(strlen($number) == 9)
							$number = "0" . $number;
						  db_insert('vat_btw_numbers')
							->fields(array(
									'number' => $number,
									'lang' => in_array(strtoupper($row[1]), array('F', 'FR')) ? 'FR' : 'NL',
									'is_active' => 0,
									'week' => $week,
									'created' => date('Y-m-d H:i:s')
							))
							->execute();

							$i++;
						}
					}

					$make_active_link = l('make them active', 'admin/config/vat/btw_number/0/-/' . $week);
					drupal_set_message($i . ' records have been imported successfully! But they are not active yet. Please ' . $make_active_link . ' !');
				}
				else {
					drupal_set_message(t("Error: Upload failed!"), 'error');
				}
			}
		}
		else {
			drupal_set_message(t("Error: Invalid file"), 'error');
		}
	}
	
  return TRUE;
}
function vat_admin_btw_number_import_form_back($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/btw_number';
}



// PARTICIPANTS ------------------------------------

// participant list
function vat_admin_participant_list($lang = '-', $week = '-', $keywords = '') {

	$output = drupal_render(drupal_get_form('vat_admin_participant_filter_form', $lang, $week, $keywords));

	$header = array(
		array('data' => 'ID', 'field' => 'id', 'sort' => 'asc'),
		array('data' => 'First name', 'field' => 'first_name', 'sort' => 'asc'),
		array('data' => 'Last name', 'field' => 'last_name', 'sort' => 'asc'),
		// array('data' => 'Company', 'field' => 'company', 'sort' => 'asc'),
		array('data' => 'Email', 'field' => 'email', 'sort' => 'asc'),
		array('data' => 'VAT number', 'field' => 'btw_number', 'sort' => 'asc'),
		array('data' => 'Week', 'field' => 'week', 'sort' => 'asc'),
		array('data' => 'Language', 'field' => 'lang', 'sort' => 'asc'),
		array('data' => 'Created on', 'field' => 'created', 'sort' => 'asc'),
		array('data' => 'Operators')
	);

	$select = db_select('vat_participants', 'p')
		->fields('p')
		->extend('TableSort')
		->extend('PagerDefault')->limit(100);

	if ($lang != '-') {	
		$select->condition('p.lang', $lang);
	}

	if ($week != '-') {	
		$select->condition('p.week', $week);
	}

	if ($keywords) {
		$or = db_or()
		->condition('p.first_name', '%' . $keywords . '%', 'LIKE')
		->condition('p.last_name', '%' . $keywords . '%', 'LIKE')
		// ->condition('p.company', '%' . $keywords . '%', 'LIKE')
		->condition('p.email', '%' . $keywords . '%', 'LIKE')
		->condition('p.btw_number', '%' . $keywords . '%', 'LIKE');
		
		$select->condition($or);
	}

	$results = $select->orderByHeader($header)->execute();	

	$rows = array();
	foreach ($results as $data) {
		list($t_week, $t_year) = explode('-', $data->week);
		$data_week = _vat_get_week_text($t_week, $t_year);	

		$row = array(
			check_plain($data->id), 
			check_plain($data->first_name), 
			check_plain($data->last_name), 
			// check_plain($data->company), 
			check_plain($data->email),
			check_plain($data->btw_number), 
			check_plain($data_week), 
			check_plain(strtoupper($data->lang)), 
			check_plain($data->created),
			l('Detail', 'admin/config/vat/participant/' . $data->id . '/detail') . ' | ' . ($data->is_winner ? '<span style="color:red">Winner</span>' : l(t('Set winner'), 'admin/config/vat/participant/' . $data->id . '/set_winner'))
		);
		$rows[] = $row;
	}
	
	$output .= theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No participant found.')));
	$output .= theme('pager', array('tags' => array()));
	
	return $output;
}
function vat_admin_participant_filter_form($form, &$form_state, $lang = '-', $week = '-', $keywords = '') {
	$form['#attributes'] = array('class' => 'search-form');
	
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Filter by')
	);
	
	$form['basic']['inline'] = array('#prefix' => '<div class="container-inline">', '#suffix' => '</div>');
	
	$form['basic']['inline']['filter'] = array(
		'#type' => 'textfield',
		'#title' => 'Keyword:',
		'#default_value' => $keywords,
		'#maxlength' => 128,
		'#size' => 25,
	);

	$week_list = _vat_generate_week_list();
	$form['basic']['inline']['week'] = array(
		'#type' => 'select',
		'#title' => t('Week:'),
		'#options' => array('-' => t('All')) + $week_list,
		'#default_value' => $week,
	);
	
	$form['basic']['inline']['lang'] = array(
		'#type' => 'select',
		'#title' => t('Language:'),
		'#options' => array(
			'-' => t('All'),
			'NL' => t('NL'),
			'FR' => t('FR'),
		),
		'#default_value' => $lang,
	);

	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_participant_filter_form_submit'),
	);
		
	if (!empty($keywords) || $week != '-' || $lang != '-') {
		$form['basic']['inline']['reset'] = array(
			'#type' => 'submit',
			'#value' => t('Reset'),
			'#submit' => array('vat_admin_participant_filter_form_reset'),
		);
	}
	return $form;
}
function vat_admin_participant_filter_form_submit($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/participant/list/'. trim($form_state['values']['lang']) . '/' . trim($form_state['values']['week']) . '/' . trim($form_state['values']['filter']);
}
function vat_admin_participant_filter_form_reset($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/participant/list';
}

function vat_admin_participant_detail($id) {
	$output = '
	<form>
	<div class="form form-layout-default clear-block rubik-processed">
		<div class="column-main"><div class="column-wrapper clear-block"><table>';

	$result = db_query("SELECT p.* FROM {vat_participants} p WHERE p.id = :id LIMIT 1", array(':id' => $id));
	if ($row = $result->fetchAssoc()) {
		
		list($t_week, $t_year) = explode('-', $row['week']);
		$record_week = _vat_get_week_text($t_week, $t_year);

		$output .= "
			<tr><th>ID:</th><td>" . $id . "</td></tr>
			<tr><th>First name:</th><td>" . $row['first_name'] . "</td></tr>
			<tr><th>Last name:</th><td>" . $row['last_name'] . "</td></tr>
			<tr><th>Company:</th><td>" . $row['company'] . "</td></tr>
			<tr><th>Email:</th><td>" . $row['email'] . "</td></tr>
			<tr><th>VAT Number:</th><td>" . $row['btw_number'] . "</td></tr>
			<tr><th>Street:</th><td>" . $row['street'] . "</td></tr>
			<tr><th>Street Number:</th><td>" . $row['street_number'] . "</td></tr>
			<tr><th>Postcode:</th><td>" . $row['postcode'] . "</td></tr>
			<tr><th>City:</th><td>" . $row['city'] . "</td></tr>
			<tr><th>Phone:</th><td>" . $row['phone'] . "</td></tr>
			<tr><th>Fax:</th><td>" . $row['fax'] . "</td></tr>
			<tr><th>Website:</th><td>" . $row['website'] . "</td></tr>
			<tr><th>Activity:</th><td>" . $row['activity'] . "</td></tr>
			<tr><th>Comment:</th><td>" . $row['comment'] . "</td></tr>
			<tr><th>Optin:</th><td>" . ($row['optin'] ? 'Yes' : 'No') . "</td></tr>
			<tr><th>Week:</th><td>" . $record_week . "</td></tr>
			<tr><th>Is winner:</th><td>" . ($row['is_winner'] ? 'Yes' : 'No') . "</td></tr>
			<tr><th>Language:</th><td>" . $row['lang'] . "</td></tr>
			<tr><th>Created on:</th><td>" . $row['created'] . "</td></tr>
		";
	}

	$output .= '
			</table></div></div>
		<div class="column-side"><div class="column-wrapper clear-block"><div class="buttons"><input type="button" class="form-submit" value="' . t('Back') . '" onclick="history.back(1)" ></div></div></div>
	</div>
	</form>';
	
	return $output;
}

function vat_admin_participant_set_winner_confirm($form, &$form_state, $id) {
  $form['set_winner'] = array(
    '#type' => 'value',
    '#value' => $id,
  );
  return confirm_form(
    $form,
    t('Are you sure you want to set this participant to be a winner?'),
    'admin/config/vat/participant',
    '',
    t('Ok'),
    t('Cancel')
  );
}
function vat_admin_participant_set_winner_confirm_submit($form, &$form_state) { 
  $record = $form_state['values']['set_winner'];
  if ($record) {
    $num_set_winner = db_update('vat_participants')
    ->fields(array(
	    'is_winner' => 1
	  ))
    ->condition('id', $record)
    ->execute();
      
    drupal_set_message('The participant has been set to be a winner!');
  }
  $form_state['redirect'] = "admin/config/vat/participant";
}

// export participant
function vat_admin_participant_export() {
	drupal_set_title(check_plain('Export participants list to CSV file'));

	$upoad_folder = drupal_get_path('module', 'vat') . '/exports/';
	if (!is_writable($upoad_folder))
		drupal_set_message('Warning: Folder "' . $upoad_folder . '" is not writable. Our file will not be able to store there.', 'warning');

	$output = drupal_render(drupal_get_form('vat_admin_participant_export_form'));
	return $output;
}
function vat_admin_participant_export_form($form, &$form_state) {
	$form['#attributes'] = array('class' => 'search-form');
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Export participants list to CSV file')
	);

	$form['basic']['inline']['lang'] = array(
		'#type' => 'select',
		'#title' => t('Select language:'),
		'#options' => array(
			'NL' => t('NL'),
			'FR' => t('FR')
		),
	);

	$week_list = _vat_generate_week_list();
	$form['basic']['inline']['week'] = array(
		'#type' => 'select',
		'#title' => t('Week:'),
		'#options' => $week_list,
		'#default_value' => date('W-Y'),
	);

	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_participant_export_form_submit'),
	);
		
	$form['basic']['inline']['back'] = array(
		'#type' => 'submit',
		'#value' => t('Back'),
		'#submit' => array('vat_admin_participant_export_form_back'),
	);
	return $form;
}
function vat_admin_participant_export_form_submit($form, &$form_state) {
	if ($form_state['submitted']) {
		$lang = $form_state['input']['lang'];
		$week = $form_state['input']['week'];

		$sql = "SELECT * FROM {vat_participants} WHERE `lang` = :lang AND `week` = :week AND `is_active` = :is_active";
		$result = db_query($sql, array(':lang' => strtoupper($lang), ':week' => $week, ':is_active' => 1));

		$file = drupal_get_path('module', 'vat') . '/exports/' . rand() . '_participants_' . date('Ymd') . '_' . $lang .  '_' . $week . '.csv';
		$fd = fopen($file, "a");

		$separator = ',';
		$headers =
			'First name' . $separator .
			'Last name' . $separator .
			'Company' . $separator .
			'Email' . $separator .
			'VAT Number' . $separator .
			'Street' . $separator .
			'Street Number' . $separator .
			'Postcode' . $separator .
			'City' . $separator .
			'Phone' . $separator .
			'Fax' . $separator .
			'Website' . $separator .
			'Activity' . $separator .
			'Comment' . $separator .
			'Optin' . $separator .
			'Week' . $separator .
			'Is winner' . $separator .
			'Language' . $separator .
			'Created on';

		fwrite($fd, $headers . "\n");

		foreach ($result as $record) {
			list($t_week, $t_year) = explode('-', $record->week);
			$record_week = _vat_get_week_text($t_week, $t_year);

			$entry = array($record->first_name, $record->last_name, $record->company, $record->email, $record->btw_number, $record->street, $record->street_number, $record->postcode, $record->city, $record->phone, $record->fax, $record->website, $record->activity, $record->comment, $record->optin ? 'Yes' : 'No', $record_week, $record->is_winner ? 'Yes' : 'No', $record->lang, $record->created);
			fputcsv($fd, $entry, $separator);
		}

		fclose($fd);

		header('Content-Type: application/csv');
		header('Content-Disposition: attachment; filename=' . basename($file));
		header('Pragma: no-cache');
		readfile($file);
		exit;
	}
	
  return TRUE;
}
function vat_admin_participant_export_form_back($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/participant';
}




// WINNER ------------------------------------

// winner list
function vat_admin_winner_list($lang = '-', $week = '-', $keywords = '') {

	$output = drupal_render(drupal_get_form('vat_admin_winner_filter_form', $lang, $week, $keywords));
	
	$header = array(
		array('data' => 'ID', 'field' => 'id', 'sort' => 'asc'),
		array('data' => 'First name', 'field' => 'first_name', 'sort' => 'asc'),
		array('data' => 'Last name', 'field' => 'last_name', 'sort' => 'asc'),
		// array('data' => 'Company', 'field' => 'company', 'sort' => 'asc'),
		array('data' => 'Email', 'field' => 'email', 'sort' => 'asc'),
		array('data' => 'VAT number', 'field' => 'btw_number', 'sort' => 'asc'),
		array('data' => 'Week', 'field' => 'week', 'sort' => 'asc'),
		array('data' => 'Language', 'field' => 'lang', 'sort' => 'asc'),
		array('data' => 'Created on', 'field' => 'created', 'sort' => 'asc'),
	);

	$select = db_select('vat_participants', 'p')
		->fields('p')
		->condition('p.is_winner', 1)
		->extend('TableSort')
		->extend('PagerDefault')->limit(100);

	if ($lang != '-') {	
		$select->condition('p.lang', $lang);
	}

	if ($week != '-') {	
		$select->condition('p.week', $week);
	}

	if ($keywords) {
		$or = db_or()
		->condition('p.first_name', '%' . $keywords . '%', 'LIKE')
		->condition('p.last_name', '%' . $keywords . '%', 'LIKE')
		// ->condition('p.company', '%' . $keywords . '%', 'LIKE')
		->condition('p.email', '%' . $keywords . '%', 'LIKE')
		->condition('p.btw_number', '%' . $keywords . '%', 'LIKE');
		
		$select->condition($or);
	}

	$results = $select->orderByHeader($header)->execute();	

	$rows = array();
	foreach ($results as $data) {
		list($t_week, $t_year) = explode('-', $data->week);
		$data_week = _vat_get_week_text($t_week, $t_year);	

		$row = array(
			check_plain($data->id), 
			check_plain($data->first_name), 
			check_plain($data->last_name), 
			// check_plain($data->company), 
			check_plain($data->email),
			check_plain($data->btw_number), 
			check_plain($data_week), 
			check_plain(strtoupper($data->lang)), 
			check_plain($data->created)
		);
		$rows[] = $row;
	}
	
	$output .= theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No winner found.')));
	$output .= theme('pager', array('tags' => array()));
	
	return $output;
}
function vat_admin_winner_filter_form($form, &$form_state, $lang = '-', $week = '-', $keywords = '') {
	$form['#attributes'] = array('class' => 'search-form');
	
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Filter by')
	);
	
	$form['basic']['inline'] = array('#prefix' => '<div class="container-inline">', '#suffix' => '</div>');
	
	$form['basic']['inline']['filter'] = array(
		'#type' => 'textfield',
		'#title' => 'Keyword:',
		'#default_value' => $keywords,
		'#maxlength' => 128,
		'#size' => 25,
	);

	$week_list = _vat_generate_week_list();
	$form['basic']['inline']['week'] = array(
		'#type' => 'select',
		'#title' => t('Week:'),
		'#options' => array('-' => t('All')) + $week_list,
		'#default_value' => $week,
	);
	
	$form['basic']['inline']['lang'] = array(
		'#type' => 'select',
		'#title' => t('Language:'),
		'#options' => array(
			'-' => t('All'),
			'NL' => t('NL'),
			'FR' => t('FR'),
		),
		'#default_value' => $lang,
	);

	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_winner_filter_form_submit'),
		);
		
	if (!empty($keywords) || $week != '-' || $lang != '-') {
		$form['basic']['inline']['reset'] = array(
			'#type' => 'submit',
			'#value' => t('Reset'),
			'#submit' => array('vat_admin_winner_filter_form_reset'),
		);
	}
	return $form;
}
function vat_admin_winner_filter_form_submit($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/winner/list/'. trim($form_state['values']['lang']) . '/' . trim($form_state['values']['week']) . '/' . trim($form_state['values']['filter']);
}
function vat_admin_winner_filter_form_reset($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/winner/list';
}

// export winner
function vat_admin_winner_export() {
	drupal_set_title(check_plain('Export winners list to CSV file'));

	$upoad_folder = drupal_get_path('module', 'vat') . '/exports/';
	if (!is_writable($upoad_folder))
		drupal_set_message('Warning: Folder "' . $upoad_folder . '" is not writable. Our file will not be able to store there.', 'warning');

	$output = drupal_render(drupal_get_form('vat_admin_winner_export_form'));
	return $output;
}
function vat_admin_winner_export_form($form, &$form_state) {
	$form['#attributes'] = array('class' => 'search-form');
	$form['basic'] = array('#type' => 'fieldset',
		'#title' => t('Export winners list to CSV file')
	);

	$form['basic']['inline']['lang'] = array(
		'#type' => 'select',
		'#title' => t('Select language:'),
		'#options' => array(
			'NL' => t('NL'),
			'FR' => t('FR')
		),
	);

	$form['basic']['inline']['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit'),
		'#submit' => array('vat_admin_winner_export_form_submit'),
	);
		
	$form['basic']['inline']['back'] = array(
		'#type' => 'submit',
		'#value' => t('Back'),
		'#submit' => array('vat_admin_winner_export_form_back'),
	);
	return $form;
}
function vat_admin_winner_export_form_submit($form, &$form_state) {
	if ($form_state['submitted']) {
		$lang = $form_state['input']['lang'];

		$sql = "SELECT * FROM {vat_participants} WHERE `lang` = :lang AND `is_winner` = :is_winner AND `is_active` = :is_active";
		$result = db_query($sql, array(':lang' => strtoupper($lang), ':is_winner' => 1, ':is_active' => 1));

		$file = drupal_get_path('module', 'vat') . '/exports/' . rand() . '_winners_' . date('Ymd') . '_' . $lang . '.csv';
		$fd = fopen($file, "a");

		$separator = ',';
		$headers =
			'First name' . $separator .
			'Last name' . $separator .
			'Company' . $separator .
			'Email' . $separator .
			'VAT Number' . $separator .
			'Street' . $separator .
			'Street Number' . $separator .
			'Postcode' . $separator .
			'City' . $separator .
			'Phone' . $separator .
			'Fax' . $separator .
			'Website' . $separator .
			'Activity' . $separator .
			'Comment' . $separator .
			'Optin' . $separator .
			'Week' . $separator .
			'Language' . $separator .
			'Created on';

		fwrite($fd, $headers . "\n");

		foreach ($result as $record) {
			list($t_week, $t_year) = explode('-', $record->week);
			$record_week = _vat_get_week_text($t_week, $t_year);

			$entry = array($record->first_name, $record->last_name, $record->company, $record->email, $record->btw_number, $record->street, $record->street_number, $record->postcode, $record->city, $record->phone, $record->fax, $record->website, $record->activity, $record->comment, $record->optin ? 'Yes' : 'No', $record_week, $record->lang, $record->created);
			fputcsv($fd, $entry, $separator);
		}

		fclose($fd);

		header('Content-Type: application/csv');
		header('Content-Disposition: attachment; filename=' . basename($file));
		header('Pragma: no-cache');
		readfile($file);
		exit;
	}
	
  return TRUE;	
}
function vat_admin_winner_export_form_back($form, &$form_state) {
	$form_state['redirect'] = 'admin/config/vat/winner';
}

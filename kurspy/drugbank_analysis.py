import xml.etree.ElementTree as ET
import requests
import pandas as pd
import time
from collections import defaultdict

ns = {'db': 'http://www.drugbank.ca'}
UNIPROT_API = "https://www.uniprot.org/uniprot/"
DELAY = 0.5

def parse_drugbank_xml(xml_path: str):
    """
    Parsuje plik drugbank_partial.xml i wyciąga powiązania lek -> list(a) uniprot_id.
    Zwraca słownik: { 'DB00001': ['P12345', ...], ... }.
    """
    tree = ET.parse(xml_path)
    root = tree.getroot()
    
    drug_targets = defaultdict(list)

    # for drug in tqdm(root.findall('db:drug', ns), desc="Przetwarzanie leków"):
    for drug in root.findall('db:drug', ns):
        drug_id_el = drug.find('db:drugbank-id[@primary="true"]', ns)
        if drug_id_el is None:
            continue
        drug_id = drug_id_el.text

        targets = drug.findall('.//db:targets/db:target', ns)
        for target in targets:
            uniprot_id_el = target.find(
                'db:polypeptide/db:external-identifiers/'
                'db:external-identifier[db:resource="UniProtKB"]/db:identifier',
                ns
            )
            if uniprot_id_el is not None:
                drug_targets[drug_id].append(uniprot_id_el.text)
    return drug_targets


def fetch_diseases_for_uniprot_ids(drug_targets_dict):
    """
    Dla słownika { drug_id: [uniprot_id1, uniprot_id2, ...] }
    pobiera z UniProt listę ID chorób (np. diseaseId), zwraca:
    { drug_id: set(disease_ids) }.
    """
    drug_diseases = defaultdict(set)
    uniprot_cache = {}

    # for drug_id, uniprot_ids in tqdm(drug_targets_dict.items(), desc="Pobieranie danych chorób"):
    for drug_id, uniprot_ids in drug_targets_dict.items():
        for uniprot_id in uniprot_ids:
            # Sprawdź, czy nie mamy w cache
            if uniprot_id in uniprot_cache:
                diseases = uniprot_cache[uniprot_id]
            else:
                diseases = set()
                try:
                    url = f"{UNIPROT_API}{uniprot_id}.json"
                    r = requests.get(url)
                    if r.status_code == 200:
                        data = r.json()
                        for comment in data.get('comments', []):
                            if comment.get('commentType') == 'DISEASE':
                                disease_info = comment.get('disease', {})
                                diseases.add(disease_info.get('diseaseId', 'Unknown'))
                    uniprot_cache[uniprot_id] = diseases
                except Exception:
                    # Błąd połączenia itp.
                    diseases = set()
                time.sleep(DELAY)
            drug_diseases[drug_id].update(diseases)

    return drug_diseases


def create_analysis_dataframe(drug_diseases):
    """
    Z { drug_id: set(disease_ids) } buduje DataFrame z kolumnami:
      - 'Drug ID'
      - 'Liczba chorób'
      - 'Choroby' (połączone przecinkami)
    """
    analysis_data = []
    for drug_id, diseases in drug_diseases.items():
        if diseases:
            disease_str = ", ".join(sorted(diseases))
        else:
            disease_str = "Brak danych"
        analysis_data.append({
            'Drug ID': drug_id,
            'Liczba chorób': len(diseases),
            'Choroby': disease_str
        })
    df = pd.DataFrame(analysis_data)
    return df

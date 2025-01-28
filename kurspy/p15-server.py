import uvicorn
import xml.etree.ElementTree as ET
from fastapi import FastAPI
from pydantic import BaseModel
from collections import defaultdict

# Ścieżka do pliku 'drugbank_partial.xml'
XML_FILE = "drugbank_partial.xml"

# Namespace z definicji pliku DrugBank
ns = {'db': 'http://www.drugbank.ca'}

# --- PARSOWANIE XML I LICZENIE PATHWAYS ---
tree = ET.parse(XML_FILE)
root = tree.getroot()

# Krok 1: Zebranie wszystkich DrugBank IDs
all_drug_ids = set()
for drug in root.findall('db:drug', ns):
    primary_id = drug.find('db:drugbank-id[@primary="true"]', ns)
    if primary_id is not None:
        all_drug_ids.add(primary_id.text)

# Krok 2: Inicjalizacja licznika (dla każdego leku ustawiamy 0)
drug_pathway_counts = defaultdict(int)
for drug_id in all_drug_ids:
    drug_pathway_counts[drug_id] = 0

# Krok 3: Zliczanie, w ilu pathway’ach występuje dany lek
for pathway in root.findall('.//db:pathway', ns):
    for drug in pathway.findall('db:drugs/db:drug', ns):
        drug_id = drug.find('db:drugbank-id', ns).text
        if drug_id in drug_pathway_counts:
            drug_pathway_counts[drug_id] += 1

# --- FASTAPI – SERWER ---
app = FastAPI(
    title="Drug Pathway Count API",
    description="Serwer zwraca liczbę ścieżek metabolicznych (pathways) związanych z danym lekiem z pliku drugbank_partial.xml.",
    version="1.0.0",
)

# Model danych przychodzących w zapytaniu POST
class DrugRequest(BaseModel):
    drug_id: str

@app.post("/get_pathway_count")
def get_pathway_count(request_data: DrugRequest):
    """
    Zwraca liczbę szlaków metabolicznych, w których dany lek (drug_id) występuje.
    """
    drug_id = request_data.drug_id
    count = drug_pathway_counts.get(drug_id, 0)  # 0, jeśli nie ma w słowniku
    return {"drug_id": drug_id, "pathway_count": count}

if __name__ == "__main__":
    # Uruchamianie serwera: python server.py
    uvicorn.run(app, host="127.0.0.1", port=8000)

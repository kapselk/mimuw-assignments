import pytest
from drugbank_analysis import (
    parse_drugbank_xml,
    fetch_diseases_for_uniprot_ids,
    create_analysis_dataframe
)
from unittest.mock import patch, MagicMock


@pytest.fixture
def example_drugbank_xml(tmp_path):

    xml_content = """<?xml version="1.0" encoding="UTF-8"?>
    <drugbank xmlns="http://www.drugbank.ca" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" version="5.1">
      <drug type="small_molecule" created="..." updated="...">
        <drugbank-id primary="true">DBTEST001</drugbank-id>
        <name>Test Drug 1</name>
        <targets>
          <target>
            <polypeptide id="P99999">
              <external-identifiers>
                <external-identifier>
                  <resource>UniProtKB</resource>
                  <identifier>P99999</identifier>
                </external-identifier>
              </external-identifiers>
            </polypeptide>
          </target>
        </targets>
      </drug>
      <drug type="small_molecule" created="..." updated="...">
        <drugbank-id primary="true">DBTEST002</drugbank-id>
        <name>Test Drug 2</name>
        <targets>
          <target>
            <polypeptide id="Q88888">
              <external-identifiers>
                <external-identifier>
                  <resource>UniProtKB</resource>
                  <identifier>Q88888</identifier>
                </external-identifier>
              </external-identifiers>
            </polypeptide>
          </target>
        </targets>
      </drug>
    </drugbank>
    """
    # Zapis do pliku w folderze tymczasowym
    test_file = tmp_path / "test_drugbank.xml"
    test_file.write_text(xml_content, encoding="utf-8")
    return str(test_file)


def test_parse_drugbank_xml(example_drugbank_xml):

    drug_targets = parse_drugbank_xml(example_drugbank_xml)
    # Oczekujemy, że słownik ma klucze: DBTEST001, DBTEST002
    assert "DBTEST001" in drug_targets
    assert "DBTEST002" in drug_targets

    # Sprawdzamy czy klucze mają oczekiwaną listę uniprot_id
    assert drug_targets["DBTEST001"] == ["P99999"]
    assert drug_targets["DBTEST002"] == ["Q88888"]


@patch("drugbank_analysis.requests.get")
def test_fetch_diseases_for_uniprot_ids(mock_get):

    # Zasymulujmy odpowiedź JSON dla UniProt ID = "P99999"
    mock_response_1 = MagicMock()
    mock_response_1.status_code = 200
    mock_response_1.json.return_value = {
        "comments": [
            {
                "commentType": "DISEASE",
                "disease": {"diseaseId": "Disease:1234"}
            }
        ]
    }
    # Zasymulujmy odpowiedź JSON dla UniProt ID = "Q88888"
    mock_response_2 = MagicMock()
    mock_response_2.status_code = 200
    mock_response_2.json.return_value = {
        "comments": [
            {
                "commentType": "DISEASE",
                "disease": {"diseaseId": "Disease:9876"}
            }
        ]
    }

    # Kolejność wywołań: najpierw dla P99999, potem dla Q88888
    mock_get.side_effect = [mock_response_1, mock_response_2]

    drug_targets_dict = {
        "DBTEST001": ["P99999"],
        "DBTEST002": ["Q88888"]
    }

    drug_diseases = fetch_diseases_for_uniprot_ids(drug_targets_dict)

    # Sprawdzamy czy requesty zostały wywołane
    assert mock_get.call_count == 2
    # Sprawdzamy wynik
    assert drug_diseases["DBTEST001"] == {"Disease:1234"}
    assert drug_diseases["DBTEST002"] == {"Disease:9876"}


def test_create_analysis_dataframe():
    drug_diseases = {
        "DBTEST001": {"Disease:1234", "Disease:5678"},
        "DBTEST002": set(),
        "DBTEST003": {"Disease:9999"}
    }
    df = create_analysis_dataframe(drug_diseases)
    # Sprawdzamy kolumny
    assert set(df.columns) == {"Drug ID", "Liczba chorób", "Choroby"}

    # Szukamy wiersza dla DBTEST001
    row_1 = df[df["Drug ID"] == "DBTEST001"].iloc[0]
    assert row_1["Liczba chorób"] == 2
    assert "Disease:1234" in row_1["Choroby"]
    assert "Disease:5678" in row_1["Choroby"]

    # DBTEST002 ma 0 chorób
    row_2 = df[df["Drug ID"] == "DBTEST002"].iloc[0]
    assert row_2["Liczba chorób"] == 0
    assert row_2["Choroby"] == "Brak danych"

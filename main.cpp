#include <QAbstractListModel>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <filesystem>

class ICandidate {
public:
        virtual const std::string &string() const = 0;
        virtual double Evaluate(const std::string &i_query) = 0;
        virtual double GetScore(const std::string &i_query) = 0;
};

class Candidate : public ICandidate {
public:
        Candidate(const std::filesystem::path &i_absPath) : m_filename(i_absPath.filename().string())
        {
                m_scoreDp.resize(m_filename.size());
                for (std::vector<double> &scoreDp : m_scoreDp) {
                        scoreDp.resize(m_filename.size(), k_lowestScore);
                }
        }

        double Evaluate(const std::string &i_query) override
        {
                return EvaluateImpl(i_query, m_filename);
        }

        double GetScore(const std::string &i_query) override
        {
                if (i_query.size() > m_filename.size())
                        return k_lowestScore;
                return *std::max_element(m_scoreDp[i_query.size() - 1].begin(), m_scoreDp[i_query.size() - 1].end());
        }

        const std::string &string() const override
        {
                return m_filename;
        }

private:
        double EvaluateImpl(const std::string_view &i_query, const std::string_view &i_filename)
        {
                if (i_query.size() > m_filename.size() || i_query.empty() || i_filename.empty())
                        return 0.;

                double &dp = m_scoreDp[i_query.size() - 1][i_filename.size() - 1];
                if (dp != k_lowestScore)
                        return dp;

                dp = std::max(std::max(std::max(-2 + EvaluateImpl(i_query, i_filename.substr(0, i_filename.size() - 1)),
                                                -2 + EvaluateImpl(i_query.substr(0, i_query.size() - 1), i_filename)),
                                       EvaluateImpl(i_query.substr(0, i_query.size() - 1),
                                                    i_filename.substr(0, i_filename.size() - 1)) +
                                               (i_query.back() == i_filename.back() ? 3. : -3.)),
                              0.);
                return dp;
        }

private:
        std::filesystem::path m_absPath;
        std::string m_filename;
        std::vector<std::vector<double>> m_scoreDp;

        static constexpr auto k_lowestScore = -std::numeric_limits<double>::infinity();
};

class ExecutableCandidate : public Candidate {
public:
        ExecutableCandidate(const std::filesystem::path &i_absPath) : Candidate(i_absPath)
        {
        }
};

class FileCandidate : public Candidate {
public:
        FileCandidate(const std::filesystem::path &i_absPath) : Candidate(i_absPath)
        {
        }
};

class CandidateFactory {
public:
        static std::unique_ptr<ICandidate> CreateCandidate(const std::filesystem::path &i_path)
        {
                std::filesystem::perms perms = std::filesystem::status(i_path).permissions();
                if (((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none) &&
                    ((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none) &&
                    ((perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none)) {
                        return std::make_unique<ExecutableCandidate>(i_path);
                } else {
                        return std::make_unique<FileCandidate>(i_path);
                }
        }
};

class CandidateModel : public QAbstractListModel {
        Q_OBJECT;

public:
        CandidateModel(QObject *i_parent = nullptr) : QAbstractListModel(i_parent)
        {
        }
        CandidateModel(std::vector<std::unique_ptr<ICandidate>> &&i_candidates)
                : QAbstractListModel(nullptr), m_candidates(std::move(i_candidates))
        {
        }

private:
        int rowCount(const QModelIndex &i_parent = QModelIndex()) const override
        {
                return m_candidates.size();
        }

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
        {
                if (!index.isValid() || index.row() >= m_candidates.size())
                        return {};
                if (role == Qt::DisplayRole)
                        return QString::fromStdString(m_candidates.at(index.row())->string());
                return {};
        }

private:
        std::vector<std::unique_ptr<ICandidate>> m_candidates;
};

int main(int argc, char *argv[])
{
        QGuiApplication app(argc, argv);
        QQmlApplicationEngine engine;

        std::vector<std::filesystem::path> paths;
        paths.emplace_back("/usr/share/applications");
        paths.emplace_back(std::getenv("HOME"));
        std::string PATH = std::getenv("PATH");
        while (!PATH.empty()) {
                size_t delim = PATH.find(':');
                if (delim == PATH.npos) {
                        if (!PATH.empty())
                                paths.emplace_back(PATH);
                        break;
                }
                std::string path = PATH.substr(0, delim);
                if (!path.empty())
                        paths.emplace_back(path);
                PATH = PATH.substr(delim + 1);
        }

        std::vector<std::unique_ptr<ICandidate>> candidates;
        std::for_each(paths.begin(), paths.end(), [&candidates](const std::filesystem::path &path) {
                for (const std::filesystem::directory_entry &entry :
                     std::filesystem::recursive_directory_iterator(path)) {
                        candidates.emplace_back(CandidateFactory::CreateCandidate(entry));
                }
        });

        const char *query = "C++";
        for (const auto &candidate : candidates)
                candidate->Evaluate(query);
        std::stable_sort(candidates.begin(), candidates.end(),
                         [query](const auto &a, const auto &b) { return a->GetScore(query) > b->GetScore(query); });

        CandidateModel candidateModel(std::move(candidates));
        engine.rootContext()->setContextProperty("candidateModel", &candidateModel);
        engine.load("main.qml");
        return app.exec();
}

#include "main.moc"

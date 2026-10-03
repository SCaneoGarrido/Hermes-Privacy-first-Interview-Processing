import { Route, Routes } from "react-router-dom";
import { Layout } from "./components/Layout";
import { InterviewsListPage } from "./pages/InterviewsListPage";
import { NewInterviewPage } from "./pages/NewInterviewPage";
import { InterviewDetailPage } from "./pages/InterviewDetailPage";
import { TranscriptReaderPage } from "./pages/TranscriptReaderPage";

export default function App() {
  return (
    <Layout>
      <Routes>
        <Route path="/" element={<InterviewsListPage />} />
        <Route path="/interviews/new" element={<NewInterviewPage />} />
        <Route path="/interviews/:id" element={<InterviewDetailPage />} />
        <Route path="/interviews/:id/transcript" element={<TranscriptReaderPage />} />
      </Routes>
    </Layout>
  );
}
